// ============================================================
// SECTION 0: Constants, hardware addresses, colors, game defines
// ============================================================

#define VGA_PIXEL_BUFFER  0xC8000000
#define VGA_CTRL_PTR      0xFF203020
#define PS2_BASE          0xFF200100
#define HEX_BASE          0xFF200020

#define SCREEN_W 320
#define SCREEN_H 240

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define YELLOW  0xFFE0
#define BLUE    0x001F
#define CYAN    0x07FF
#define GREY    0x8410
#define GREEN   0x07E0
#define MAGENTA 0xF81F
#define ORANGE  0xFCA0

#define ROWS 6
#define COLS 7
#define EMPTY 0
#define PLAYER 1
#define AI 2

#define CELL_SIZE 28
#define CELL_GAP  2
#define CIRCLE_R  12
#define BOARD_X   54
#define BOARD_Y   30

#define MAX_DEPTH 7

#define STATE_TITLE 0
#define STATE_PLAYING 1

#define NUM_COLORS 6

// ============================================================
// SECTION 1: Global variables
// ============================================================

int board[ROWS][COLS];
int prev_board[ROWS][COLS];
int prev_turn;
int current_col;
int game_over;   // 0=playing, 1=player won, 2=AI won, 3=draw
int turn;        // 1=player, 2=AI
int turn_count;
int pixel_buffer_start;
int extended_key;
int board_drawn;

int app_state;          // STATE_TITLE or STATE_PLAYING
int player_color_idx;   // index into color_options
short int player_color; // actual color value

static const short int color_options[NUM_COLORS] = {
    0xF800, // RED
    0x07E0, // GREEN
    0x07FF, // CYAN
    0xF81F, // MAGENTA
    0xFCA0, // ORANGE
    0xFFFF  // WHITE
};

static const char *color_names[NUM_COLORS] = {
    "Red", "Green", "Cyan", "Magenta", "Orange", "White"
};

// ============================================================
// SECTION 2: Bitmap font table (5x7)
// ============================================================

static const unsigned char font5x7[][7] = {
    // 0x20 = space (index 0)
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    // 0x21 = '!' (index 1)
    {0x04,0x04,0x04,0x04,0x04,0x00,0x04},
    // 0x22 = '"' (index 2)
    {0x0A,0x0A,0x00,0x00,0x00,0x00,0x00},
    // 0x23 = '#' (index 3)
    {0x0A,0x1F,0x0A,0x0A,0x1F,0x0A,0x00},
    // 0x24 = '$' (index 4)
    {0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04},
    // 0x25 = '%' (index 5)
    {0x18,0x19,0x02,0x04,0x08,0x13,0x03},
    // 0x26 = '&' (index 6)
    {0x08,0x14,0x14,0x08,0x15,0x12,0x0D},
    // 0x27 = '\'' (index 7)
    {0x04,0x04,0x00,0x00,0x00,0x00,0x00},
    // 0x28 = '(' (index 8)
    {0x02,0x04,0x08,0x08,0x08,0x04,0x02},
    // 0x29 = ')' (index 9)
    {0x08,0x04,0x02,0x02,0x02,0x04,0x08},
    // 0x2A = '*' (index 10)
    {0x04,0x15,0x0E,0x1F,0x0E,0x15,0x04},
    // 0x2B = '+' (index 11)
    {0x00,0x04,0x04,0x1F,0x04,0x04,0x00},
    // 0x2C = ',' (index 12)
    {0x00,0x00,0x00,0x00,0x00,0x04,0x08},
    // 0x2D = '-' (index 13)
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
    // 0x2E = '.' (index 14)
    {0x00,0x00,0x00,0x00,0x00,0x00,0x04},
    // 0x2F = '/' (index 15)
    {0x01,0x01,0x02,0x04,0x08,0x10,0x10},
    // 0x30 = '0' (index 16)
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    // 0x31 = '1' (index 17)
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    // 0x32 = '2' (index 18)
    {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},
    // 0x33 = '3' (index 19)
    {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E},
    // 0x34 = '4' (index 20)
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    // 0x35 = '5' (index 21)
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    // 0x36 = '6' (index 22)
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
    // 0x37 = '7' (index 23)
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    // 0x38 = '8' (index 24)
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    // 0x39 = '9' (index 25)
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    // 0x3A = ':' (index 26)
    {0x00,0x00,0x04,0x00,0x04,0x00,0x00},
    // 0x3B = ';' (index 27)
    {0x00,0x00,0x04,0x00,0x04,0x04,0x08},
    // 0x3C = '<' (index 28)
    {0x02,0x04,0x08,0x10,0x08,0x04,0x02},
    // 0x3D = '=' (index 29)
    {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00},
    // 0x3E = '>' (index 30)
    {0x08,0x04,0x02,0x01,0x02,0x04,0x08},
    // 0x3F = '?' (index 31)
    {0x0E,0x11,0x01,0x02,0x04,0x00,0x04},
    // 0x40 = '@' (index 32)
    {0x0E,0x11,0x17,0x15,0x17,0x10,0x0E},
    // 0x41 = 'A' (index 33)
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
    // 0x42 = 'B' (index 34)
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    // 0x43 = 'C' (index 35)
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
    // 0x44 = 'D' (index 36)
    {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
    // 0x45 = 'E' (index 37)
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    // 0x46 = 'F' (index 38)
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    // 0x47 = 'G' (index 39)
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E},
    // 0x48 = 'H' (index 40)
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    // 0x49 = 'I' (index 41)
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
    // 0x4A = 'J' (index 42)
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C},
    // 0x4B = 'K' (index 43)
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    // 0x4C = 'L' (index 44)
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    // 0x4D = 'M' (index 45)
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},
    // 0x4E = 'N' (index 46)
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    // 0x4F = 'O' (index 47)
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    // 0x50 = 'P' (index 48)
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    // 0x51 = 'Q' (index 49)
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
    // 0x52 = 'R' (index 50)
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    // 0x53 = 'S' (index 51)
    {0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E},
    // 0x54 = 'T' (index 52)
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    // 0x55 = 'U' (index 53)
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    // 0x56 = 'V' (index 54)
    {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04},
    // 0x57 = 'W' (index 55)
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11},
    // 0x58 = 'X' (index 56)
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    // 0x59 = 'Y' (index 57)
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
    // 0x5A = 'Z' (index 58)
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
    // 0x5B = '[' (index 59)
    {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E},
    // 0x5C = '\\' (index 60)
    {0x10,0x10,0x08,0x04,0x02,0x01,0x01},
    // 0x5D = ']' (index 61)
    {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E},
    // 0x5E = '^' (index 62)
    {0x04,0x0A,0x11,0x00,0x00,0x00,0x00},
    // 0x5F = '_' (index 63)
    {0x00,0x00,0x00,0x00,0x00,0x00,0x1F},
    // 0x60 = '`' (index 64)
    {0x08,0x04,0x00,0x00,0x00,0x00,0x00},
    // 0x61 = 'a' (index 65)
    {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F},
    // 0x62 = 'b' (index 66)
    {0x10,0x10,0x1E,0x11,0x11,0x11,0x1E},
    // 0x63 = 'c' (index 67)
    {0x00,0x00,0x0E,0x11,0x10,0x11,0x0E},
    // 0x64 = 'd' (index 68)
    {0x01,0x01,0x0F,0x11,0x11,0x11,0x0F},
    // 0x65 = 'e' (index 69)
    {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E},
    // 0x66 = 'f' (index 70)
    {0x06,0x08,0x08,0x1E,0x08,0x08,0x08},
    // 0x67 = 'g' (index 71)
    {0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E},
    // 0x68 = 'h' (index 72)
    {0x10,0x10,0x1E,0x11,0x11,0x11,0x11},
    // 0x69 = 'i' (index 73)
    {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E},
    // 0x6A = 'j' (index 74)
    {0x02,0x00,0x06,0x02,0x02,0x12,0x0C},
    // 0x6B = 'k' (index 75)
    {0x10,0x10,0x12,0x14,0x18,0x14,0x12},
    // 0x6C = 'l' (index 76)
    {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E},
    // 0x6D = 'm' (index 77)
    {0x00,0x00,0x1A,0x15,0x15,0x15,0x15},
    // 0x6E = 'n' (index 78)
    {0x00,0x00,0x1E,0x11,0x11,0x11,0x11},
    // 0x6F = 'o' (index 79)
    {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E},
    // 0x70 = 'p' (index 80)
    {0x00,0x00,0x1E,0x11,0x1E,0x10,0x10},
    // 0x71 = 'q' (index 81)
    {0x00,0x00,0x0F,0x11,0x0F,0x01,0x01},
    // 0x72 = 'r' (index 82)
    {0x00,0x00,0x16,0x19,0x10,0x10,0x10},
    // 0x73 = 's' (index 83)
    {0x00,0x00,0x0F,0x10,0x0E,0x01,0x1E},
    // 0x74 = 't' (index 84)
    {0x08,0x08,0x1E,0x08,0x08,0x09,0x06},
    // 0x75 = 'u' (index 85)
    {0x00,0x00,0x11,0x11,0x11,0x11,0x0F},
    // 0x76 = 'v' (index 86)
    {0x00,0x00,0x11,0x11,0x11,0x0A,0x04},
    // 0x77 = 'w' (index 87)
    {0x00,0x00,0x11,0x11,0x15,0x15,0x0A},
    // 0x78 = 'x' (index 88)
    {0x00,0x00,0x11,0x0A,0x04,0x0A,0x11},
    // 0x79 = 'y' (index 89)
    {0x00,0x00,0x11,0x11,0x0F,0x01,0x0E},
    // 0x7A = 'z' (index 90)
    {0x00,0x00,0x1F,0x02,0x04,0x08,0x1F},
};

// ============================================================
// SECTION 3: VGA drawing functions
// ============================================================

void plot_pixel(int x, int y, short int color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    volatile short int *vga = (volatile short int *)(pixel_buffer_start + (y << 10) + (x << 1));
    *vga = color;
}

void clear_screen(void) {
    int x, y;
    for (y = 0; y < SCREEN_H; y++)
        for (x = 0; x < SCREEN_W; x++)
            plot_pixel(x, y, BLACK);
}

void draw_rect(int x1, int y1, int x2, int y2, short int color) {
    int x, y;
    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++)
            plot_pixel(x, y, color);
}

void draw_circle(int cx, int cy, int r, short int color) {
    int x, y;
    for (y = -r; y <= r; y++)
        for (x = -r; x <= r; x++)
            if (x * x + y * y <= r * r)
                plot_pixel(cx + x, cy + y, color);
}

void draw_char(int x, int y, char c, short int color) {
    int idx;
    if (c >= 0x20 && c <= 0x7A)
        idx = c - 0x20;
    else
        return;

    int row, col;
    for (row = 0; row < 7; row++) {
        unsigned char line = font5x7[idx][row];
        for (col = 0; col < 5; col++) {
            if (line & (0x10 >> col))
                plot_pixel(x + col, y + row, color);
        }
    }
}

void draw_string(int x, int y, const char *str, short int color) {
    while (*str) {
        draw_char(x, y, *str, color);
        x += 6;
        str++;
    }
}

// ============================================================
// SECTION 3b: Title screen
// ============================================================

// Draw a horizontal line
void draw_hline(int x1, int x2, int y, short int color) {
    int x;
    for (x = x1; x <= x2; x++) plot_pixel(x, y, color);
}

void draw_title_screen(void) {
    clear_screen();

    // Title: "CONNECT 4" centered (9 chars * 6px = 54px wide, center of 320 = 133)
    draw_string(133, 15, "CONNECT 4", WHITE);

    // Decorative line under title
    draw_hline(100, 220, 25, GREY);

    // Row of small alternating pieces as decoration
    int dx;
    for (dx = 0; dx < 7; dx++) {
        int cx = 82 + dx * 24;
        short int c = (dx % 2 == 0) ? RED : YELLOW;
        draw_circle(cx, 45, 8, c);
    }

    // Another decorative line
    draw_hline(100, 220, 58, GREY);

    // "Choose your color" label centered
    draw_string(103, 75, "Choose color:", WHITE);

    // Color preview circle
    draw_circle(160, 100, 10, player_color);

    // Color name with arrows: "< Red >"
    // Clear area and draw
    draw_rect(90, 116, 230, 126, BLACK);
    draw_string(120, 117, "<", WHITE);
    // Center the color name (max 7 chars = 42px, center around 160)
    int name_len = 0;
    const char *nm = color_names[player_color_idx];
    while (nm[name_len]) name_len++;
    int name_x = 160 - (name_len * 6) / 2;
    draw_string(name_x, 117, color_names[player_color_idx], player_color);
    draw_string(192, 117, ">", WHITE);

    // Play button — rounded look with border
    draw_rect(120, 140, 200, 160, BLUE);
    draw_rect(121, 141, 199, 159, BLUE);  // slightly inset for clean edges
    draw_string(142, 147, "PLAY", WHITE);

    // Instructions at bottom
    draw_hline(80, 240, 175, GREY);
    draw_string(82, 185, "Left/Right: color", GREY);
    draw_string(82, 197, "Enter: start game", GREY);

    // Small credit
    draw_string(112, 225, "ECE243 2026", GREY);
}

void update_title_color(void) {
    player_color = color_options[player_color_idx];

    // Redraw color circle
    draw_circle(160, 100, 10, player_color);

    // Redraw color name
    draw_rect(90, 116, 230, 126, BLACK);
    draw_string(120, 117, "<", WHITE);
    int name_len = 0;
    const char *nm = color_names[player_color_idx];
    while (nm[name_len]) name_len++;
    int name_x = 160 - (name_len * 6) / 2;
    draw_string(name_x, 117, color_names[player_color_idx], player_color);
    draw_string(192, 117, ">", WHITE);
}

// ============================================================
// SECTION 3c: Board drawing with partial updates
// ============================================================

void draw_cell(int r, int c) {
    int cx = BOARD_X + c * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
    int cy = BOARD_Y + r * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
    draw_circle(cx, cy, CIRCLE_R, BLACK);
    if (board[r][c] == PLAYER)
        draw_circle(cx, cy, CIRCLE_R - 1, player_color);
    else if (board[r][c] == AI)
        draw_circle(cx, cy, CIRCLE_R - 1, YELLOW);
}

void draw_cursor(int col) {
    draw_rect(BOARD_X - 2, BOARD_Y - 12, BOARD_X + COLS * (CELL_SIZE + CELL_GAP) + 1, BOARD_Y - 3, BLACK);
    int cx = BOARD_X + col * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
    int ay = BOARD_Y - 10;
    int i, w;
    short int col_color = (turn == PLAYER) ? player_color : YELLOW;
    for (i = 0; i < 6; i++) {
        w = 5 - i;
        draw_rect(cx - w, ay + i, cx + w, ay + i, col_color);
    }
}

void draw_status(const char *msg, short int color) {
    draw_rect(0, 226, SCREEN_W - 1, 239, BLACK);
    draw_string(90, 226, msg, color);
    draw_string(70, 234, "Press R to restart", GREY);
}

void draw_board(void) {
    int r, c;

    if (!board_drawn) {
        draw_rect(0, 0, SCREEN_W - 1, 14, BLACK);
        draw_string(120, 2, "CONNECT 4", WHITE);

        draw_rect(BOARD_X - 2, BOARD_Y - 2,
                  BOARD_X + COLS * (CELL_SIZE + CELL_GAP) + 1,
                  BOARD_Y + ROWS * (CELL_SIZE + CELL_GAP) + 1, BLUE);

        for (r = 0; r < ROWS; r++)
            for (c = 0; c < COLS; c++)
                draw_cell(r, c);

        int num_y = BOARD_Y + ROWS * (CELL_SIZE + CELL_GAP) + 4;
        for (c = 0; c < COLS; c++) {
            int nx = BOARD_X + c * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2 - 2;
            draw_char(nx, num_y, '1' + c, WHITE);
        }

        board_drawn = 1;
    } else {
        for (r = 0; r < ROWS; r++)
            for (c = 0; c < COLS; c++)
                if (board[r][c] != prev_board[r][c])
                    draw_cell(r, c);
    }

    if (turn != prev_turn) {
        draw_rect(0, 14, SCREEN_W - 1, 24, BLACK);
        if (!game_over) {
            if (turn == PLAYER)
                draw_string(110, 15, "Your turn", CYAN);
            else
                draw_string(110, 15, "AI thinking...", YELLOW);
        }
        prev_turn = turn;
    }

    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            prev_board[r][c] = board[r][c];

    draw_cursor(current_col);
    update_hex(turn_count);
}

// ============================================================
// SECTION 3d: Smooth drop animation
// ============================================================

void delay_loop(int count) {
    volatile int d;
    for (d = 0; d < count; d++);
}

// Get center Y of a given row's cell hole
int cell_center_y(int row) {
    return BOARD_Y + row * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
}

// Get center X of a given col's cell hole
int cell_center_x(int col) {
    return BOARD_X + col * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
}

// Find which row a y-coordinate is closest to (for this column)
// Returns the row index (0 to ROWS-1), clamped
int nearest_row(int y) {
    int best_row = 0;
    int best_dist = 99999;
    int r;
    for (r = 0; r < ROWS; r++) {
        int cy = cell_center_y(r);
        int dist = y - cy;
        if (dist < 0) dist = -dist;
        if (dist < best_dist) {
            best_dist = dist;
            best_row = r;
        }
    }
    return best_row;
}

// Draw a colored piece ONLY within the black hole of a specific cell.
// This means: for each pixel of the piece circle, only draw it if that
// pixel is also within the cell hole circle. Blue frame is never touched.
void draw_piece_in_hole(int hole_row, int col, int piece_cy, int piece_r, short int color) {
    int hcx = cell_center_x(col);
    int hcy = cell_center_y(hole_row);
    int hr = CIRCLE_R; // hole radius

    // We draw the intersection of two circles:
    // the piece (centered at hcx, piece_cy with radius piece_r)
    // and the hole (centered at hcx, hcy with radius hr)

    // Bounding box is the hole circle (we only care about pixels in the hole)
    int px, py;
    for (py = -hr; py <= hr; py++) {
        for (px = -hr; px <= hr; px++) {
            int sx = hcx + px;
            int sy = hcy + py;

            // Must be inside the hole circle
            if (px * px + py * py > hr * hr) continue;

            // Must be inside the piece circle
            int dx = sx - hcx;
            int dy = sy - piece_cy;
            if (dx * dx + dy * dy > piece_r * piece_r) continue;

            plot_pixel(sx, sy, color);
        }
    }
}

// Clear any trace of the animated piece from a hole by redrawing it
// as black, then redrawing any existing board piece in that cell
void clear_hole(int row, int col) {
    int hcx = cell_center_x(col);
    int hcy = cell_center_y(row);
    // Fill hole with black
    draw_circle(hcx, hcy, CIRCLE_R, BLACK);
    // Redraw existing piece if any
    if (board[row][col] == PLAYER)
        draw_circle(hcx, hcy, CIRCLE_R - 1, player_color);
    else if (board[row][col] == AI)
        draw_circle(hcx, hcy, CIRCLE_R - 1, YELLOW);
}

// Draw the animated piece at position piece_cy, visible through all
// holes it overlaps in this column
void draw_anim(int col, int piece_cy, int piece_r, short int color) {
    int r;
    for (r = 0; r < ROWS; r++) {
        int hcy = cell_center_y(r);
        int dist = piece_cy - hcy;
        if (dist < 0) dist = -dist;
        // Does the piece circle overlap this hole circle?
        if (dist < piece_r + CIRCLE_R) {
            draw_piece_in_hole(r, col, piece_cy, piece_r, color);
        }
    }
}

// Erase the animated piece: clear only holes that old position touched
// but new position does NOT touch. This prevents flicker on shared holes.
void erase_anim_selective(int col, int old_cy, int new_cy, int piece_r) {
    int r;
    for (r = 0; r < ROWS; r++) {
        int hcy = cell_center_y(r);
        // Was this hole touched by old position?
        int old_dist = old_cy - hcy;
        if (old_dist < 0) old_dist = -old_dist;
        int old_touched = (old_dist < piece_r + CIRCLE_R);

        // Is this hole touched by new position?
        int new_dist = new_cy - hcy;
        if (new_dist < 0) new_dist = -new_dist;
        int new_touched = (new_dist < piece_r + CIRCLE_R);

        // Only clear holes that old touched but new doesn't
        if (old_touched && !new_touched) {
            clear_hole(r, col);
        }
    }
}

// Erase all holes touched by the piece at this position
void erase_anim_all(int col, int piece_cy, int piece_r) {
    int r;
    for (r = 0; r < ROWS; r++) {
        int hcy = cell_center_y(r);
        int dist = piece_cy - hcy;
        if (dist < 0) dist = -dist;
        if (dist < piece_r + CIRCLE_R) {
            clear_hole(r, col);
        }
    }
}

void drop_piece_animated(int col, int piece) {
    int target_row = get_open_row(col);
    if (target_row < 0) return;

    short int color = (piece == PLAYER) ? player_color : YELLOW;
    int target_y = cell_center_y(target_row);
    int start_y = cell_center_y(0); // start at top row hole
    int piece_r = CIRCLE_R - 1;

    // Phase 1: Fall with gravity
    int y = start_y;
    int prev_y = start_y;
    int velocity = 0;
    int first = 1;

    while (y < target_y) {
        if (!first) {
            // Draw at new position FIRST, then erase only leftover holes
            draw_anim(col, y, piece_r, color);
            erase_anim_selective(col, prev_y, y, piece_r);
        } else {
            draw_anim(col, y, piece_r, color);
        }
        delay_loop(45000);

        first = 0;
        prev_y = y;
        velocity += 2;
        y += velocity;
        if (y > target_y) y = target_y;
    }

    // Snap to target — draw new, clean old
    draw_anim(col, target_y, piece_r, color);
    erase_anim_selective(col, prev_y, target_y, piece_r);
    delay_loop(30000);

    // Phase 2: Bounce
    int bounce_height = (CELL_SIZE + CELL_GAP); // ~30px, almost one full cell
    int b;
    for (b = 0; b < 3 && bounce_height >= 4; b++) {
        int prev_offset = 0;

        // Rise
        int offset;
        for (offset = 2; offset <= bounce_height; offset += 2) {
            int new_y = target_y - offset;
            int old_y = target_y - prev_offset;
            draw_anim(col, new_y, piece_r, color);
            erase_anim_selective(col, old_y, new_y, piece_r);
            delay_loop(36000);
            prev_offset = offset;
        }

        // Fall back down
        for (offset = bounce_height - 2; offset >= 0; offset -= 2) {
            int new_y = target_y - offset;
            int old_y = target_y - prev_offset;
            draw_anim(col, new_y, piece_r, color);
            erase_anim_selective(col, old_y, new_y, piece_r);
            delay_loop(36000);
            prev_offset = offset;
        }

        // Ensure we end exactly at target_y
        draw_anim(col, target_y, piece_r, color);
        erase_anim_selective(col, target_y - prev_offset, target_y, piece_r);

        bounce_height = bounce_height * 2 / 5; // ~30 -> 12 -> 4
    }

    // Final cleanup: only clear the target cell hole, then commit
    erase_anim_all(col, target_y, piece_r);

    // Commit piece to board
    board[target_row][col] = piece;
    turn_count++;

    // Draw the final piece properly
    draw_cell(target_row, col);
    prev_board[target_row][col] = piece;
}

// ============================================================
// SECTION 4: PS/2 keyboard handler
// ============================================================

int read_ps2(void) {
    volatile int *ps2 = (volatile int *)PS2_BASE;
    int data = *ps2;

    if (!(data & 0x8000))
        return -1;

    return data & 0xFF;
}

// ============================================================
// SECTION 5: Game logic
// ============================================================

int is_valid_col(int col) {
    return (col >= 0 && col < COLS && board[0][col] == EMPTY);
}

int get_open_row(int col) {
    int r;
    for (r = ROWS - 1; r >= 0; r--) {
        if (board[r][col] == EMPTY)
            return r;
    }
    return -1;
}

void drop_piece(int col, int piece) {
    int r = get_open_row(col);
    if (r >= 0) {
        board[r][col] = piece;
        turn_count++;
    }
}

int check_win(int piece) {
    int r, c;

    for (r = 0; r < ROWS; r++)
        for (c = 0; c <= COLS - 4; c++)
            if (board[r][c] == piece && board[r][c+1] == piece &&
                board[r][c+2] == piece && board[r][c+3] == piece)
                return 1;

    for (c = 0; c < COLS; c++)
        for (r = 0; r <= ROWS - 4; r++)
            if (board[r][c] == piece && board[r+1][c] == piece &&
                board[r+2][c] == piece && board[r+3][c] == piece)
                return 1;

    for (r = 0; r <= ROWS - 4; r++)
        for (c = 0; c <= COLS - 4; c++)
            if (board[r][c] == piece && board[r+1][c+1] == piece &&
                board[r+2][c+2] == piece && board[r+3][c+3] == piece)
                return 1;

    for (r = 3; r < ROWS; r++)
        for (c = 0; c <= COLS - 4; c++)
            if (board[r][c] == piece && board[r-1][c+1] == piece &&
                board[r-2][c+2] == piece && board[r-3][c+3] == piece)
                return 1;

    return 0;
}

int is_board_full(void) {
    int c;
    for (c = 0; c < COLS; c++)
        if (board[0][c] == EMPTY)
            return 0;
    return 1;
}

void reset_game(void) {
    int r, c;
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++) {
            board[r][c] = EMPTY;
            prev_board[r][c] = -1;
        }
    current_col = 3;
    game_over = 0;
    turn = PLAYER;
    prev_turn = -1;
    turn_count = 0;
    extended_key = 0;
    board_drawn = 0;
}

// ============================================================
// SECTION 6: AI (minimax with alpha-beta pruning)
// ============================================================

int evaluate_window(int window[4], int piece) {
    int opp = (piece == AI) ? PLAYER : AI;
    int count_piece = 0, count_opp = 0, count_empty = 0;
    int i;

    for (i = 0; i < 4; i++) {
        if (window[i] == piece) count_piece++;
        else if (window[i] == opp) count_opp++;
        else count_empty++;
    }

    if (count_piece == 4) return 100000;
    if (count_piece == 3 && count_empty == 1) return 50;
    if (count_piece == 2 && count_empty == 2) return 5;

    if (count_opp == 4) return -100000;
    if (count_opp == 3 && count_empty == 1) return -50;

    return 0;
}

int score_board(int piece) {
    int score = 0;
    int r, c, i;
    int window[4];

    for (r = 0; r < ROWS; r++)
        if (board[r][3] == piece)
            score += 3;

    for (r = 0; r < ROWS; r++) {
        for (c = 0; c <= COLS - 4; c++) {
            for (i = 0; i < 4; i++) window[i] = board[r][c + i];
            score += evaluate_window(window, piece);
        }
    }

    for (c = 0; c < COLS; c++) {
        for (r = 0; r <= ROWS - 4; r++) {
            for (i = 0; i < 4; i++) window[i] = board[r + i][c];
            score += evaluate_window(window, piece);
        }
    }

    for (r = 0; r <= ROWS - 4; r++) {
        for (c = 0; c <= COLS - 4; c++) {
            for (i = 0; i < 4; i++) window[i] = board[r + i][c + i];
            score += evaluate_window(window, piece);
        }
    }

    for (r = 3; r < ROWS; r++) {
        for (c = 0; c <= COLS - 4; c++) {
            for (i = 0; i < 4; i++) window[i] = board[r - i][c + i];
            score += evaluate_window(window, piece);
        }
    }

    return score;
}

static const int col_order[7] = {3, 2, 4, 1, 5, 0, 6};

int minimax(int depth, int alpha, int beta, int is_maximizing) {
    int ai_win = check_win(AI);
    int player_win = check_win(PLAYER);
    int full = is_board_full();

    if (ai_win) return 100000 + depth;
    if (player_win) return -100000 - depth;
    if (full) return 0;
    if (depth == 0) return score_board(AI);

    int i, col, row;

    if (is_maximizing) {
        int best = -9999999;
        for (i = 0; i < COLS; i++) {
            col = col_order[i];
            if (!is_valid_col(col)) continue;
            row = get_open_row(col);
            board[row][col] = AI;
            int val = minimax(depth - 1, alpha, beta, 0);
            board[row][col] = EMPTY;
            if (val > best) best = val;
            if (best > alpha) alpha = best;
            if (alpha >= beta) break;
        }
        return best;
    } else {
        int best = 9999999;
        for (i = 0; i < COLS; i++) {
            col = col_order[i];
            if (!is_valid_col(col)) continue;
            row = get_open_row(col);
            board[row][col] = PLAYER;
            int val = minimax(depth - 1, alpha, beta, 1);
            board[row][col] = EMPTY;
            if (val < best) best = val;
            if (best < beta) beta = best;
            if (alpha >= beta) break;
        }
        return best;
    }
}

int get_ai_move(void) {
    int best_col = 3;
    int best_score = -9999999;
    int i, col, row;

    for (i = 0; i < COLS; i++) {
        col = col_order[i];
        if (!is_valid_col(col)) continue;
        row = get_open_row(col);
        board[row][col] = AI;
        int val = minimax(MAX_DEPTH - 1, -9999999, 9999999, 0);
        board[row][col] = EMPTY;
        if (val > best_score) {
            best_score = val;
            best_col = col;
        }
    }

    return best_col;
}

// ============================================================
// SECTION 7: HEX display
// ============================================================

static const unsigned char hex7seg[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

void update_hex(int value) {
    volatile int *hex = (volatile int *)HEX_BASE;
    int d0 = value % 10;
    int d1 = (value / 10) % 10;
    *hex = (hex7seg[d1] << 8) | hex7seg[d0];
}

// ============================================================
// SECTION 8: Main
// ============================================================

int main(void) {
    volatile int *pixel_ctrl_ptr = (int *)VGA_CTRL_PTR;
    pixel_buffer_start = *pixel_ctrl_ptr;

    // Initialize title screen state
    app_state = STATE_TITLE;
    player_color_idx = 0;
    player_color = color_options[0];
    extended_key = 0;

    draw_title_screen();

    int break_code = 0;

    while (1) {

        // ---- TITLE SCREEN ----
        if (app_state == STATE_TITLE) {
            int key = read_ps2();
            if (key == -1) continue;

            if (break_code) {
                break_code = 0;
                extended_key = 0;
                continue;
            }
            if (key == 0xF0) {
                break_code = 1;
                continue;
            }
            if (key == 0xE0) {
                extended_key = 1;
                continue;
            }

            if (extended_key) {
                extended_key = 0;
                if (key == 0x6B) { // Left arrow
                    if (player_color_idx > 0)
                        player_color_idx--;
                    else
                        player_color_idx = NUM_COLORS - 1;
                    update_title_color();
                } else if (key == 0x74) { // Right arrow
                    player_color_idx++;
                    if (player_color_idx >= NUM_COLORS)
                        player_color_idx = 0;
                    update_title_color();
                }
                continue;
            }

            // Enter = start game
            if (key == 0x5A) {
                player_color = color_options[player_color_idx];
                app_state = STATE_PLAYING;
                reset_game();
                clear_screen();
                draw_board();
            }
            continue;
        }

        // ---- GAME OVER STATE ----
        if (game_over) {
            int key = read_ps2();
            if (key == -1) continue;
            if (break_code) {
                break_code = 0;
                continue;
            }
            if (key == 0xF0) {
                break_code = 1;
                continue;
            }
            if (key == 0xE0) continue;
            if (key == 0x2D) { // R = restart
                reset_game();
                clear_screen();
                draw_board();
            }
            if (key == 0x1B) { // Escape (S key = 0x1B) - back to title
                // Actually use T key (0x2C) for title... let's use R for restart
                // and keep it simple
            }
            continue;
        }

        // ---- PLAYER TURN ----
        if (turn == PLAYER) {
            int key = read_ps2();
            if (key == -1) continue;

            if (break_code) {
                break_code = 0;
                extended_key = 0;
                continue;
            }
            if (key == 0xF0) {
                break_code = 1;
                continue;
            }
            if (key == 0xE0) {
                extended_key = 1;
                continue;
            }

            if (extended_key) {
                extended_key = 0;
                if (key == 0x6B) {
                    if (current_col > 0) current_col--;
                    draw_cursor(current_col);
                } else if (key == 0x74) {
                    if (current_col < COLS - 1) current_col++;
                    draw_cursor(current_col);
                }
                continue;
            }

            int selected = -1;
            switch (key) {
                case 0x16: selected = 0; break;
                case 0x1E: selected = 1; break;
                case 0x26: selected = 2; break;
                case 0x25: selected = 3; break;
                case 0x2E: selected = 4; break;
                case 0x36: selected = 5; break;
                case 0x3D: selected = 6; break;
            }

            if (selected >= 0) {
                current_col = selected;
                draw_cursor(current_col);
            }

            // Enter = drop piece with animation
            if (key == 0x5A) {
                if (is_valid_col(current_col)) {
                    drop_piece_animated(current_col, PLAYER);
                    if (check_win(PLAYER)) {
                        game_over = 1;
                        draw_status("You win!", player_color);
                    } else if (is_board_full()) {
                        game_over = 3;
                        draw_status("Draw!", WHITE);
                    } else {
                        turn = AI;
                        draw_board();
                    }
                }
            }

            if (key == 0x2D) { // R
                reset_game();
                clear_screen();
                draw_board();
            }

        } else {
            // ---- AI TURN ----
            draw_board();
            int col = get_ai_move();
            drop_piece_animated(col, AI);
            turn = PLAYER;
            if (check_win(AI)) {
                game_over = 2;
                turn = AI;
                draw_status("AI wins!", YELLOW);
            } else if (is_board_full()) {
                game_over = 3;
                draw_status("Draw!", WHITE);
            } else {
                draw_board();
            }
        }
    }

    return 0;
}