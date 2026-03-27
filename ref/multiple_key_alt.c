//Alternative functions that test the drawing functionsa and use all the keys number keys to select rows
int main(void) {
    volatile int *pixel_ctrl_ptr = (int *)VGA_CTRL_PTR;
    pixel_buffer_start = *pixel_ctrl_ptr;

    int r, c;
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            board[r][c] = EMPTY;

    current_col = 3;
    extended_key = 0;
    break_code = 0;

    clear_screen();
    draw_board();

    while (1) {
        int key = read_ps2();
        if (key == -1) continue;

        if (break_code) { break_code = 0; extended_key = 0; continue; }
        if (key == 0xF0) { break_code = 1; continue; }
        if (key == 0xE0) { extended_key = 1; continue; }

        if (extended_key) {
            extended_key = 0;
            if (key == 0x6B && current_col > 0) current_col--;
            else if (key == 0x74 && current_col < COLS - 1) current_col++;
            draw_cursor(current_col);
            continue;
        }

        // Number keys 1-7 to select column
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

        // Enter to drop piece
        if (key == 0x5A && is_valid_col(current_col)) {
            int row = get_open_row(current_col);
            if (row >= 0) {
                board[row][current_col] = PLAYER;
                draw_cell(row, current_col);
            }
        }
    }

    return 0;
}


int read_ps2(void) {
    volatile int *ps2 = (volatile int *)PS2_BASE;
    int data = *ps2;
    if (!(data & 0x8000)) return -1;
    return data & 0xFF;
}	