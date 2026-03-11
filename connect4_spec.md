# ECE243 Final Project Spec: Connect 4 with AI Opponent

## Overview

Connect 4 on the DE1-SoC. Human vs AI on a 7-column, 6-row board displayed on VGA. Player selects columns via PS/2 keyboard. AI uses minimax with alpha-beta pruning. Single file: `project.c`, compatible with CPUlator (Nios V).

---

## Hardware (CPUlator-safe only)

| Device | Address | Usage |
|--------|---------|-------|
| VGA Pixel Buffer | `0xC8000000` | 320x240, 16-bit RGB565 |
| VGA Buffer Controller | `0xFF203020` | Read pixel buffer address |
| PS/2 Keyboard | `0xFF200100` | Column selection |
| HEX Displays | `0xFF200020` | Show turn count or score |

No audio, no timers, no interrupts, no double buffering.

---

## Colors

```c
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define YELLOW  0xFFE0
#define BLUE    0x001F
#define CYAN    0x07FF
#define GREY    0x8410
```

Player 1 (human) = RED. Player 2 (AI) = YELLOW. Empty = BLACK. Board frame = BLUE.

---

## Board Data

```c
#define ROWS 6
#define COLS 7
#define EMPTY 0
#define PLAYER 1
#define AI 2

int board[ROWS][COLS]; // 0=empty, 1=player, 2=AI
```

Row 0 is the TOP of the board. Pieces drop to the lowest empty row in a column.

---

## VGA Layout (320x240)

```
+------------------------------------------+
| Title: "CONNECT 4"  (y: 2-12)           |
| Turn indicator: "Your turn" / "AI..."    |
+------------------------------------------+
|                                          |
|         7x6 grid centered               |
|         (x: 54-265, y: 30-222)          |
|                                          |
|    Column numbers: 1 2 3 4 5 6 7        |
|    Cursor arrow above selected column    |
+------------------------------------------+
| Status: "You win!" / "AI wins!" / "Draw" |
| "Press R to restart"  (y: 226-238)       |
+------------------------------------------+
```

Each cell: 28x28 pixels with 2px gap. Circle diameter ~24px drawn inside each cell.

### Drawing Functions

```c
void plot_pixel(int x, int y, short int color);
void clear_screen(void);
void draw_circle(int cx, int cy, int r, short int color);  // filled circle
void draw_rect(int x1, int y1, int x2, int y2, short int color); // filled rectangle
void draw_char(int x, int y, char c, short int color);     // 5x7 bitmap font
void draw_string(int x, int y, const char *str, short int color);
void draw_board(void);      // renders entire board state to VGA
void draw_cursor(int col);  // arrow above selected column
void draw_status(const char *msg, short int color);
```

**The 5x7 bitmap font must be COMPLETE for characters: 0-9, A-Z, a-z, space, '!', ':', '.', '-'. At minimum 50 characters with real pixel data. No placeholders.**

---

## PS/2 Keyboard Input

```c
int read_ps2(void); // returns scan code byte or -1 if no data
```

**Required key mappings:**

| Key | Scan Code | Action |
|-----|-----------|--------|
| 1-7 | 0x16,0x1E,0x26,0x25,0x2E,0x36,0x3D | Select column 1-7 |
| Left arrow | 0xE0,0x6B | Move cursor left |
| Right arrow | 0xE0,0x74 | Move cursor right |
| Enter | 0x5A | Drop piece in selected column |
| R | 0x2D | Restart game |

Handle 0xF0 break codes (ignore the byte after 0xF0). Handle 0xE0 extended prefix for arrow keys.

---

## Game State

```c
int current_col;    // currently highlighted column (0-6)
int game_over;      // 0=playing, 1=player won, 2=AI won, 3=draw
int turn;           // 1=player turn, 2=AI turn
int turn_count;     // total moves made
```

---

## Core Game Functions

```c
int is_valid_col(int col);              // returns 1 if column is not full
int get_open_row(int col);              // returns lowest empty row, or -1
void drop_piece(int col, int piece);    // places piece in column
int check_win(int piece);               // returns 1 if piece has 4 in a row
int is_board_full(void);                // returns 1 if no valid moves
void reset_game(void);                  // clears board, resets all state
```

### Win Detection

Check all four directions:
- **Horizontal:** for each row, slide a window of 4 across columns
- **Vertical:** for each column, slide a window of 4 down rows
- **Diagonal (↘):** for r=0..2, c=0..3, check (r,c)(r+1,c+1)(r+2,c+2)(r+3,c+3)
- **Diagonal (↗):** for r=3..5, c=0..3, check (r,c)(r-1,c+1)(r-2,c+2)(r-3,c+3)

---

## AI: Minimax with Alpha-Beta Pruning

```c
#define MAX_DEPTH 7

int get_ai_move(void);  // returns column 0-6
```

### Internals

```c
int score_board(int piece);                    // heuristic evaluation
int evaluate_window(int window[4], int piece); // score a group of 4 cells
int minimax(int depth, int alpha, int beta, int is_maximizing);
```

**Scoring heuristic:**
- 4 in a row: +100000 (AI) or -100000 (player)
- 3 in a row + 1 empty: +50 / -50
- 2 in a row + 2 empty: +5 / -5
- Center column preference: +3 per AI piece in column 3

**Minimax behavior:**
- Terminal states: return score if win/loss/draw or depth==0
- Maximizing (AI): try all valid columns, track best score, alpha-beta prune
- Minimizing (player): try all valid columns, track worst score, alpha-beta prune
- Column ordering: try center first `{3,2,4,1,5,0,6}` for better pruning
- Undo moves after evaluating: `board[row][col] = EMPTY`

---

## Main Loop

```c
int main(void) {
    volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
    pixel_buffer_start = *pixel_ctrl_ptr;

    reset_game();
    clear_screen();
    draw_board();

    while (1) {
        if (game_over) {
            // wait for R key to restart
            int key = read_ps2();
            if (key == 0x2D) {  // R
                reset_game();
                clear_screen();
                draw_board();
            }
            continue;
        }

        if (turn == PLAYER) {
            int key = read_ps2();
            if (key == -1) continue;
            // handle column selection, cursor movement, piece drop
            // after valid drop: check win/draw, switch to AI turn
        } else {
            // AI turn
            int col = get_ai_move();
            drop_piece(col, AI);
            draw_board();
            if (check_win(AI)) {
                game_over = 2;
                draw_status("AI wins!", YELLOW);
            } else if (is_board_full()) {
                game_over = 3;
                draw_status("Draw!", WHITE);
            } else {
                turn = PLAYER;
            }
        }
    }
}
```

---

## HEX Display (bonus, simple)

Show turn count on HEX0-HEX1:

```c
void update_hex(int value);
// Write to 0xFF200020 (HEX3-0)
// Encode 2 digits using 7-segment lookup table
```

---

## File Structure (single file, this order)

```
// SECTION 0: Constants, hardware addresses, colors, game defines
// SECTION 1: Global variables (board, game state, pixel_buffer_start)
// SECTION 2: Bitmap font table (5x7, minimum 50 chars with real data)
// SECTION 3: VGA drawing functions (plot_pixel, circles, rects, text, board)
// SECTION 4: PS/2 keyboard handler
// SECTION 5: Game logic (drop_piece, check_win, is_board_full, reset_game)
// SECTION 6: AI (evaluate_window, score_board, minimax, get_ai_move)
// SECTION 7: HEX display
// SECTION 8: Main
```

---

## Testing

Claude Code must also produce `project_testable.c`:
- Identical to `project.c` but with `#define TESTING_MODE 1` at top
- `main()` removed
- All hardware functions (`plot_pixel`, `clear_screen`, `draw_*`, `read_ps2`, `update_hex`) wrapped in `#ifndef TESTING_MODE` guards (become empty no-ops when testing)
- All game logic and AI functions remain fully functional

Compile tests with: `gcc -o tests test_project.c -Wall && ./tests`

---

## NO PLACEHOLDERS RULE

Every function must be fully implemented. The font table must have real pixel data. The scan code table must be complete. No TODOs, no stubs, no `// implementation here`. Complete working code only.
