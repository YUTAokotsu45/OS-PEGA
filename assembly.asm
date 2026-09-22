format binary as 'bin'

; ==============================================================================
; PEGASUS OS - Bootloader complet (FASM)
; Disque : [Secteur 1 : boot 16-bit] [Secteur 2 : transition 32/64-bit] [Secteur 3-52 : kernel.bin] [Secteur 53 : text.txt]
; ==============================================================================

KERNEL_ADDR     = 0x1000        ; adresse de chargement du kernel
KERNEL_SECTORS  = 50            ; nombre de secteurs lus (25 Ko max)
STAGE2_ADDR     = 0x7E00        ; adresse de chargement du secteur 2
TEXT_ADDR       = 0x8000        ; adresse de chargement du fichier texte en RAM

PML4_ADDRESS    = 0x90000
PDPT_ADDRESS    = 0x91000
PD_ADDRESS      = 0x92000

; ==============================================================================
; SECTEUR 1 : BOOTLOADER 16-BIT (0x7C00)
; ==============================================================================
org 0x7C00
use16

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    ; 1. Charger le secteur 2 (transition Long Mode) a 0000:7E00
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, STAGE2_ADDR
    int 0x13
    jc disk_error

    ; 2. Charger le kernel (secteurs 3 a 52) a 0000:1000
    mov ah, 0x02
    mov al, KERNEL_SECTORS
    mov ch, 0
    mov cl, 3
    mov dh, 0
    mov dl, [boot_drive]
    mov bx, KERNEL_ADDR
    int 0x13
    jc disk_error

    ; 3. Charger le fichier texte (secteur 53) a 0000:8000
    mov ah, 0x02
    mov al, 1                   ; 1 secteur (512 octets)
    mov ch, 0                   ; Piste 0
    mov cl, 53                  ; Secteur 53 (2 + 50 + 1)
    mov dh, 0                   ; Tête 0
    mov dl, [boot_drive]
    mov bx, TEXT_ADDR
    int 0x13
    jc disk_error

    ; 4. Activer la ligne A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 5. Sauter vers le secteur 2
    jmp 0x0000:STAGE2_ADDR

; --- GESTION ERREUR DISQUE ---
disk_error:
    mov si, msg_err
print_loop:
    lodsb
    or al, al
    jz halt_system
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x0C
    int 0x10
    jmp print_loop

halt_system:
    cli
    hlt
    jmp halt_system

boot_drive db 0
msg_err    db "Erreur : Impossible de charger Pegasus OS !", 0

; Remplissage exact du secteur 1 a 512 octets
rb 510 - ($ - $$)
dw 0xAA55

; ==============================================================================
; SECTEUR 2 : MODE PROTEGE 32-BIT PUIS LONG MODE 64-BIT (0x7E00)
; ==============================================================================
org STAGE2_ADDR
use16

init_setup:
    cli
    lgdt [gdt32_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:init_32bit

; GDT 32-bit
align 8
gdt32:
    dq 0
    dw 0xFFFF, 0x0000, 0x9A00, 0x00CF      ; code 32-bit (0x08)
    dw 0xFFFF, 0x0000, 0x9200, 0x00CF      ; donnees 32-bit (0x10)
gdt32_end:

gdt32_descriptor:
    dw gdt32_end - gdt32 - 1
    dd gdt32

use32
init_32bit:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Vider les tables de pages (3 x 4096 octets = 3072 dwords)
    mov edi, PML4_ADDRESS
    xor eax, eax
    mov ecx, 3072
    rep stosd

    ; PML4[0] -> PDPT
    mov dword [PML4_ADDRESS], PDPT_ADDRESS or 0x03
    mov dword [PML4_ADDRESS + 4], 0

    ; PDPT[0] -> PD
    mov dword [PDPT_ADDRESS], PD_ADDRESS or 0x03
    mov dword [PDPT_ADDRESS + 4], 0

    ; PD[0] -> page de 2 Mo identite (0x00000000 - 0x001FFFFF)
    mov dword [PD_ADDRESS], 0x000000 or 0x83
    mov dword [PD_ADDRESS + 4], 0

    ; Charger CR3
    mov eax, PML4_ADDRESS
    mov cr3, eax

    ; Activer PAE
    mov eax, cr4
    or eax, 1 shl 5
    mov cr4, eax

    ; Activer Long Mode (EFER.LME)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 shl 8
    wrmsr

    ; Activer la pagination
    mov eax, cr0
    or eax, 1 shl 31
    mov cr0, eax

    lgdt [gdt64_descriptor]
    jmp 0x08:init_64bit

; GDT 64-bit
align 8
gdt64:
    dq 0
    dw 0x0000, 0x0000, 0x9A00, 0x00AF      ; code 64-bit (0x08)
    dw 0x0000, 0x0000, 0x9200, 0x0000      ; donnees 64-bit (0x10)
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64 - 1
    dd gdt64

use64
init_64bit:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, 0x90000

    mov rax, KERNEL_ADDR
    jmp rax

    cli
    hlt

; Remplissage exact du secteur 2 a 512 octets
rb 512 - ($ - STAGE2_ADDR)

; ==============================================================================
; SECTEUR 3+ : KERNEL.BIN
; ==============================================================================
kernel_start:
file 'kernel.bin'
kernel_end:

; Securite : le kernel doit tenir dans les secteurs lus par le bootloader
if kernel_end - kernel_start > KERNEL_SECTORS * 512
    display 'Erreur : kernel.bin est trop gros pour KERNEL_SECTORS', 13, 10
    err
end if

; Padding : le disque doit contenir exactement KERNEL_SECTORS secteurs pour le kernel
db (KERNEL_SECTORS * 512) - (kernel_end - kernel_start) dup 0

; ==============================================================================
; SECTEUR 53 : FICHIER TEXTE (Mise en mémoire à 0x8000)
; ==============================================================================
file_start:
file 'text.txt'
file_end:

; Remplissage pour compléter le secteur 53 à 512 octets
if file_end - file_start > 512
    display 'Erreur : text.txt depasse 512 octets', 13, 10
    err
end if

db 512 - (file_end - file_start) dup 0