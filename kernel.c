#define VGA_ADDRESS 0xB8000
#define RAM_MAX_ADDRESS 0x00FFFFFF  // Exemple : limite des 16 Mo de RAM
#define RAM_MIN_ADDRESS 0x00100000  // Début de la mémoire libre (après le kernel

// Palette de couleurs VGA
#define BLACK_ON_BLACK   0x00
#define BLUE_ON_BLACK    0x09  // BLEU
#define GREEN_ON_BLACK   0x0A  // VERT
#define RED_ON_BLACK     0x0C  // ROUGE
#define PINK_ON_BLACK    0x0D  // ROSE
#define YELLOW_ON_BLACK  0x0E  // JAUNE
#define WHITE_ON_BLACK   0x0F  // BLANC
#define ORANGE_ON_BLACK  0x06  // ORANGE / BRUN

// Déclarations des fonctions
void print_32(char* text, int row, int col, char color);
void clear_screen();
unsigned char PegaLine(int row);
unsigned char inb(unsigned short port);
void outb(unsigned short port, unsigned char data);
unsigned char PegaASCII();
void PegaCAPS(unsigned char key);
int PegaCONTAINER(const char *needle, const char *texte);
void PegaCUSER(int row, char *buffer);
void print_prompt(int row);
void PegaClock(char *buffer);
char* PegaFAT(int *out_index);
int PegaCADRESS (char* adress) ; 
char* PegaOSFILE (char* file_name, char* extension) ;

static unsigned char last_scancode = 0;
static unsigned char caps_lock = 0; // 0 = minuscule, 1 = majuscule

volatile char* vga = (volatile char*) VGA_ADDRESS;

const char ascii_table[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', ')', '=', '\b',
    '\t', 'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '^', '$', '\n',
    0, 'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', '%', '*',
    0, '\\', 'w', 'x', 'c', 'v', 'b', 'n', ',', ';', ':', '!', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const char shift_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0xF8, '+', '\b',
    '\t', 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0xF9, 0x9C, '\n',
    0, 'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '%', 0xE6,
    0, '>', 'W', 'X', 'C', 'V', 'B', 'N', '?', '.', '/', 0x15, 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

typedef struct {
    char name[8];        // Nom du fichier (rempli d'espaces si < 8 chars)
    char ext[3];         // Extension (ex: "TXT")
    char attributes;     // Attributs (Dossier, Lecture seule, etc.)
    char reserved[10];
    unsigned short time;
    unsigned short date;
    unsigned short cluster_low; // Le secteur où commence le contenu
    unsigned int size;         // Taille en octets
}__attribute__((packed)) PegaFileEntry ;

const char *thres_command[] = {
    "clear", "help", "echo", 
    "version", "color", "clock",
    "quote"
};

#define PROMPT_LEN 9 // Longueur de "pegasus> "

char color = GREEN_ON_BLACK;

void kmain() {
    clear_screen();

    print_32("Pegasus OS - Terminal Interactif Actif !", 0, 0, GREEN_ON_BLACK);
    
    int index_row = 2;
    print_prompt(index_row);
    int index_col = PROMPT_LEN;

    while (1) {
        unsigned char keyboard = PegaASCII();

        if (keyboard != 0) {
            if (keyboard == '\n') {
                char user_command[81];
                PegaCUSER(index_row, user_command);

                index_row++;
                index_col = 0;

                int commande_num = sizeof(thres_command) / sizeof(thres_command[0]);
                int command_found = 0;

                for (int index = 0; index < commande_num; index++) {
                    if (PegaCONTAINER(thres_command[index], user_command)) {
                        command_found = 1;

                        if (index == 0) { // clear
                            clear_screen();
                            index_row = 0;
                        }
                        else if (index == 1) { // help
                            print_32("Commandes: clear, help, echo, version, color, clock", index_row, 0, color);
                            index_row++;
                        }
                        else if (index == 2) { // echo
                            print_32(user_command + PROMPT_LEN, index_row, 0, color);
                            index_row++;
                        }
                        else if (index == 3) { // version
                            print_32("Pegasus OS v0.1 - 32-bit Protected Mode", index_row, 0, color);
                            index_row++;
                        }
                        else if (index == 4) { // color 
                            if (PegaCONTAINER("rouge", user_command) || PegaCONTAINER("ROUGE", user_command)) {
                                color = RED_ON_BLACK;
                                print_32("Couleur changee en ROUGE !", index_row, 0, color);
                            }
                            else if (PegaCONTAINER("bleu", user_command) || PegaCONTAINER("BLEU", user_command)) {
                                color = BLUE_ON_BLACK;
                                print_32("Couleur changee en BLEU !", index_row, 0, color);
                            }
                            else if (PegaCONTAINER("vert", user_command) || PegaCONTAINER("VERT", user_command)) {
                                color = GREEN_ON_BLACK;
                                print_32("Couleur changee en VERT !", index_row, 0, color);
                            }
                            else if (PegaCONTAINER("rose", user_command) || PegaCONTAINER("ROSE", user_command)) {
                                color = PINK_ON_BLACK;
                                print_32("Couleur changee en ROSE !", index_row, 0, color);
                            }
                            else if (PegaCONTAINER("jaune", user_command) || PegaCONTAINER("JAUNE", user_command)) {
                                color = YELLOW_ON_BLACK;
                                print_32("Couleur changee en JAUNE !", index_row, 0, color);
                            }
                            else if (PegaCONTAINER("orange", user_command) || PegaCONTAINER("ORANGE", user_command)) {
                                color = ORANGE_ON_BLACK;
                                print_32("Couleur changee en ORANGE !", index_row, 0, color);
                            }
                            else {
                                print_32("Usage: color <rouge|bleu|vert|rose|jaune|orange>", index_row, 0, WHITE_ON_BLACK);
                            }
                            index_row++;
                        }
                        else if (index == 5) { // clock
                            char time_str[9]; // "HH:MM:SS" + '\0'
                            PegaClock(time_str);

                            print_32("Heure RTC : ", index_row, 0, color);
                            print_32(time_str, index_row, 12, color);
                            index_row++;
                        }

                        else if (index == 6) {
                            int taille = 0 ;
                            char* my_text = PegaOSFILE("caca.txt", "txt") ;
                            print_32(my_text, index_row++ , index_col, color) ;
                            
                        }

                        break;
                    }
                }

                if (!command_found && user_command[PROMPT_LEN] != '\0' && user_command[PROMPT_LEN] != ' ') {
                    print_32("Commande inconnue. Tapez 'help'.", index_row, 0, color);
                    index_row++;
                }

                // Sécurité écran
                if (index_row >= 24) {
                    clear_screen();
                    index_row = 0;
                }

                // Réaffiche le prompt sur la nouvelle ligne
                print_prompt(index_row);
                index_col = PROMPT_LEN;
            }
            else if (keyboard == '\b') {
                if (index_col > PROMPT_LEN) {
                    index_col--;
                    char space[2] = {' ', 0};
                    print_32(space, index_row, index_col, color);
                }
            }
            else {
                if (index_col < 79) {
                    char str[2] = {keyboard, 0};
                    print_32(str, index_row, index_col, color);
                    index_col++;
                }
            }
        }
    }
}

// Fonction pour récupérer l'heure formatée en HH:MM:SS depuis le CMOS
void PegaClock(char *buffer) {
    // Attendre que le CMOS ne soit pas en train de faire une mise à jour
    outb(0x70, 0x0A);
    while (inb(0x71) & 0x80);

    // Heures (0x04)
    outb(0x70, 0x04);
    unsigned char h = inb(0x71);

    // Minutes (0x02)
    outb(0x70, 0x02);
    unsigned char m = inb(0x71);

    // Secondes (0x00)
    outb(0x70, 0x00);
    unsigned char s = inb(0x71);

    // Formate les paires de chiffres BCD
    buffer[0] = '0' + (h >> 4);
    buffer[1] = '0' + (h & 0x0F);
    buffer[2] = ':';
    buffer[3] = '0' + (m >> 4);
    buffer[4] = '0' + (m & 0x0F);
    buffer[5] = ':';
    buffer[6] = '0' + (s >> 4);
    buffer[7] = '0' + (s & 0x0F);
    buffer[8] = '\0';
}

void print_prompt(int row) {
    print_32("pegasus> ", row, 0, GREEN_ON_BLACK);
}

int PegaCADRESS (char* adress) {

    int addr = (unsigned int)adress ;

    if (
        addr > RAM_MAX_ADDRESS || 
        addr < RAM_MIN_ADDRESS ||
        addr == 0 
    ) {
        return 0 ; 
    }

    return 1 ;
}



void clear_screen() {
    volatile char* vga = (volatile char*) VGA_ADDRESS;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vga[i] = ' ';
        vga[i + 1] = color;
    }
}

void print_32(char* text, int row, int col, char color) {
    volatile char* vga = (volatile char*) VGA_ADDRESS;
    int offset = (row * 80 + col) * 2;
    int i = 0;

    while (text[i] != 0) {
        vga[offset] = text[i];
        vga[offset + 1] = color;
        offset += 2;
        i++;
    }
}

void outb(unsigned short port, unsigned char data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

static inline unsigned short inw(unsigned short port) {
    unsigned short ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#define IDE_DATA        0x1F0
#define IDE_SECTOR_CNT  0x1F2
#define IDE_LBA_LOW     0x1F3
#define IDE_LBA_MID     0x1F4
#define IDE_LBA_HIGH    0x1F5
#define IDE_DRIVE       0x1F6
#define IDE_COMMAND     0x1F7
#define IDE_STATUS      0x1F7

void ide_wait_ready() {
    // On attend que le bit BSY (Busy / Occupé) devienne 0
    while ((inb(IDE_STATUS) & 0x80) != 0);
    while ((inb(IDE_STATUS) & 0x08) == 0);
}

char PegaDISK(unsigned int lba, unsigned short* buffer) {
    ide_wait_ready();

    // On configure le contrôleur de disque (Mode LBA 28-bits)
    outb(IDE_DRIVE,      0xE0 | ((lba >> 24) & 0x0F)); 
    outb(IDE_SECTOR_CNT, 1);                           // On veut lire 1 seul secteur
    outb(IDE_LBA_LOW,    (unsigned char)lba);          
    outb(IDE_LBA_MID,    (unsigned char)(lba >> 8));   
    outb(IDE_LBA_HIGH,   (unsigned char)(lba >> 16));  

    // On envoie la commande magique : 0x20 signifie "READ SECTORS"
    outb(IDE_COMMAND,    0x20);

    ide_wait_ready() ; 

    for (int i = 0 ; i < 256 ; i ++) {
        buffer[i] = inw(IDE_DATA) ;
    }
}

char* PegaOSFILE (char* file_name, char* extension) {

    unsigned char unbuffer[256] ;

    // J'ai retiré le "char * disk =" car PegaDISK ne renvoie rien, elle remplit le buffer
    PegaDISK(500, (unsigned short*)unbuffer) ; 

    PegaFileEntry * table = (PegaFileEntry*) unbuffer ;

    for (int index = 0 ; index < 16 ; index ++) {
        if (table[index].name == file_name) {
            if (table[index].ext == extension) {
                // --- CODE AJOUTÉ CI-DESSOUS ---
                // 1. On lit le secteur du fichier trouvé directement dans unbuffer
                PegaDISK(table[index].cluster_low, (unsigned short*)unbuffer);
                
                return (char*)unbuffer;
            }
        }
    }
    return 0; // Si rien n'est trouvé
}


void PegaCAPS(unsigned char key) {
    if (key == 0x3A && key != last_scancode) {
        caps_lock = !caps_lock;
    }
}

unsigned char PegaASCII() {
    unsigned char key = inb(0x60);

    PegaCAPS(key);

    if (key >= 0x80) {
        last_scancode = 0;
        return 0;
    }

    if (key == last_scancode) {
        return 0;
    }

    last_scancode = key;

    if (key == 0x3A) {
        return 0;
    }

    if (key < 128) {
        return caps_lock ? shift_ascii[key] : ascii_table[key];
    }

    return 0;
}

void PegaCUSER(int row, char *buffer) {
    volatile char* vga = (volatile char*) VGA_ADDRESS;
    int last_non_space = 0;

    for (int index = 0; index < 80; index++) {
        int offset = (row * 80 + index) * 2;
        buffer[index] = vga[offset];
        if (vga[offset] != ' ') {
            last_non_space = index;
        }
    }

    buffer[last_non_space + 1] = '\0';
}

unsigned char PegaLine(int row) {
    volatile char* vga = (volatile char*) VGA_ADDRESS;
    unsigned char last_col = 0;

    for (int col = 0; col < 80; col++) {
        int offset = (row * 80 + col) * 2;
        if (vga[offset] != ' ') {
            last_col = col;
        }
    }

    return last_col;
}

int PegaCONTAINER(const char *needle, const char *texte) {
    if (!*needle) return 1;
    for (; *texte; texte++) {
        const char *h = texte;
        const char *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return 1;
    }
    return 0;
}

char* PegaFAT(int *out_index) {
    char *file_data = (char *)0x8000;
    int index = 0;

    if (file_data[0] == '\0') {
        *out_index = 0;
        return "Fichier vide";
    }

    // Compte le nombre de caractères
    while (file_data[index] != '\0' && index < 512) {
        index++;
    }

    *out_index = index;

    return file_data;
}