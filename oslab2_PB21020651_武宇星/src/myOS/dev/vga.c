extern void outb (unsigned short int port_to, unsigned char value);
extern unsigned char inb(unsigned short int port_from);

#define VGA_BASE_ADDR 0xb8000
#define ROWS 25
#define COLS 80

#define BLACK   0x0
#define BLUE    0x1
#define GREEN   0x2
#define CYAN    0x3
#define RED     0x4
#define MAGENTA 0x5
#define BROWN   0x6
#define LGRAY   0x7

#define BRIGHT  0x8
#define DGRAY   BRIGHT & BLACK
#define LBLUE   BRIGHT & BLUE
#define LGREEN  BRIGHT & GREEN
#define LCYAN   BRIGHT & CYAN
#define LRED    BRIGHT & RED
#define PINK    BRIGHT & MAGENTA
#define YELLOW  BRIGHT & BROWN
#define WHITE   BRIGHT & LGRAY

#define BLINK   0x80

static void update_cursor(unsigned int row, unsigned int col) {
    unsigned int cursor_loc = row * 80 + col;

    outb(0x3D4, 14);
    outb(0x3D5, cursor_loc >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, cursor_loc & 0xff);
}

unsigned char* vga_address(int row, int col) {
    unsigned char* addr = (unsigned char*)VGA_BASE_ADDR;
    return addr + row * COLS * 2 + col * 2;
}

void write_char(char ch, char color, int row, int col) {
    unsigned char* addr = vga_address(row, col);
    *addr++ = ch;
    *addr = color;
}

void clear_char(int row, int col) {
    unsigned char* addr = vga_address(row, col);
    *addr++ = 0;
    *addr = 0x7;
}

void clear_last_row(void) {
    int col;
    for (col = 0; col < COLS; col++) clear_char(ROWS - 1, col);
}

void scroll_one_row(void) {
    int i;
    unsigned char* ptr = (unsigned char*)VGA_BASE_ADDR;
    unsigned char* next_row_ptr = vga_address(1, 0);

    for (i = 0; i < (ROWS - 1) * COLS; i++) {
        *ptr++ = *next_row_ptr++;
        *ptr++ = *next_row_ptr++;
    }
    clear_last_row();
}

void clear_screen(void) {
    unsigned char* ptr = (unsigned char*)VGA_BASE_ADDR;
    unsigned int row, col;
    for (row = 0; row < ROWS; row++) {
        for (col = 0; col < COLS; col++) {
            (*ptr++) = 0;
            (*ptr++) = 0x7;
        }
    }
    update_cursor(0, 0);
return;
}

void append2screen(char* str, int color) {
    char ch, *current = str;
    unsigned int cursor_loc;
    int row, col;
    outb(0x3D4, 14);
    cursor_loc = inb(0x3D5) << 8;
    outb(0x3D4, 15);
    cursor_loc |= inb(0x3D5);

    row = cursor_loc / 80;
    col = cursor_loc % 80;

    while (ch = *current++) {
        if (ch != '\n') {
            write_char(ch, color, row, col++);
        } else {
            col = 0;
            row++;
        }

        if (col >= COLS) {
            col = 0;
            row++;
        }
        if (row >= ROWS) {
            scroll_one_row();
            row = ROWS - 1;
        }
    }
    update_cursor(row, col);
}
