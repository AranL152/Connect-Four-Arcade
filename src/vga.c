// Minimal Connect 4 MVP - Board display + piece dropping

#define VGA_PIXEL_BUFFER  0xC8000000
#define VGA_CTRL_PTR      0xFF203020
#define PS2_BASE          0xFF200100

#define SCREEN_W 320
#define SCREEN_H 240

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define BLUE    0x001F
#define CYAN    0x07FF

#define ROWS 6
#define COLS 7
#define EMPTY 0
#define PLAYER 1

#define CELL_SIZE 28
#define CELL_GAP  2
#define CIRCLE_R  12
#define BOARD_X   54
#define BOARD_Y   30

int board[ROWS][COLS];
int current_col;
int pixel_buffer_start;
int extended_key;
int break_code;

// 5x7 bitmap font - only chars we need (0-7, space, and a few letters)
static const unsigned char font5x7[][7] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space
    {0x04,0x04,0x04,0x04,0x04,0x00,0x04}, // !
    {0x0A,0x0A,0x00,0x00,0x00,0x00,0x00}, // "
    {0x0A,0x1F,0x0A,0x0A,0x1F,0x0A,0x00}, // #
    {0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04}, // $
    {0x18,0x19,0x02,0x04,0x08,0x13,0x03}, // %
    {0x08,0x14,0x14,0x08,0x15,0x12,0x0D}, // &
    {0x04,0x04,0x00,0x00,0x00,0x00,0x00}, // '
    {0x02,0x04,0x08,0x08,0x08,0x04,0x02}, // (
    {0x08,0x04,0x02,0x02,0x02,0x04,0x08}, // )
    {0x04,0x15,0x0E,0x1F,0x0E,0x15,0x04}, // *
    {0x00,0x04,0x04,0x1F,0x04,0x04,0x00}, // +
    {0x00,0x00,0x00,0x00,0x00,0x04,0x08}, // ,
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}, // -
    {0x00,0x00,0x00,0x00,0x00,0x00,0x04}, // .
    {0x01,0x01,0x02,0x04,0x08,0x10,0x10}, // /
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
    {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F}, // 2
    {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}, // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
};

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
    if (c >= 0x20 && c <= 0x37)
        idx = c - 0x20;
    else
        return;
    int row, col;
    for (row = 0; row < 7; row++) {
        unsigned char line = font5x7[idx][row];
        for (col = 0; col < 5; col++)
            if (line & (0x10 >> col))
                plot_pixel(x + col, y + row, color);
    }
}


int is_valid_col(int col) {
    return col >= 0 && col < COLS && board[0][col] == EMPTY;
}

int get_open_row(int col) {
    int r;
    for (r = ROWS - 1; r >= 0; r--)
        if (board[r][col] == EMPTY)
            return r;
    return -1;
}

void draw_cell(int r, int c) {
    int cx = BOARD_X + c * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
    int cy = BOARD_Y + r * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
    draw_circle(cx, cy, CIRCLE_R, BLACK);
    if (board[r][c] == PLAYER)
        draw_circle(cx, cy, CIRCLE_R - 1, RED);
}

void draw_cursor(int col) {
    draw_rect(BOARD_X - 2, BOARD_Y - 12, BOARD_X + COLS * (CELL_SIZE + CELL_GAP) + 1, BOARD_Y - 3, BLACK);
    int cx = BOARD_X + col * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
    int ay = BOARD_Y - 10;
    int i, w;
    for (i = 0; i < 6; i++) {
        w = 5 - i;
        draw_rect(cx - w, ay + i, cx + w, ay + i, RED);
    }
}

void draw_board(void) {
    int r, c;
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

    draw_cursor(current_col);
}

int main(){

    //Need to code new main to test once we have finished logic code/other stuff
    //for this TA check in
}

	