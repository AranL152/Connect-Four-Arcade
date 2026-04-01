// CPULator Requires all code in 1 file and address macros explicitly declared
//As of right now, this is the only correct version
//Other files are incomplete

//--------------------------------------------------kb_decoder.C-------------------------------------------------------//
#define LEDR_BASE 0xFF200000
#define PS2_BASE 0xFF200100

// Data registers
#define PS2_DATA ((volatile int*)PS2_BASE)
#define PS2_CTRL ((volatile int*)(PS2_BASE + 4))
#define LEDR_DEST ((volatile int*)(LEDR_BASE))

// Constant names for input PS/2 codes for relevant inputs
#define LEFT_CODE 0x6B
#define RIGHT_CODE 0x74
#define ENTER_CODE 0x5A
#define BREAK_CODE 0xF0
#define EXT_CODE 0xE0

#define IRQ22 0x80000016

// Interval timer registers at 0xFF202000
// Status bit 0 = TO (timeout), write 0 to clear
// Control: bit0=ITO, bit1=CONT, bit2=START, bit3=STOP
#define TIMER_BASE_ADDR 0xFF202000
#define TIMER_STATUS ((volatile int*)TIMER_BASE_ADDR)
#define TIMER_CTRL ((volatile int*)(TIMER_BASE_ADDR + 4))
#define TIMER_PERIOD_LO ((volatile int*)(TIMER_BASE_ADDR + 8))
#define TIMER_PERIOD_HI ((volatile int*)(TIMER_BASE_ADDR + 12))
#define TIMER_IRQ 0x80000010  // timer mcause value (mie bit 16)
#define TURN_TIME 10          // seconds per move

// Define internal alias for keyboard input
typedef enum { NONE, LEFT, RIGHT, ENTER } kbInputKey;
volatile kbInputKey currInput = NONE;

// Timer globals - ISR decrements every second
volatile int timer_seconds = TURN_TIME;
volatile int timer_expired = 0;

// Function definitions

// Required definition for interrupt handler
// Handles both keyboard and timer interrupts
static void handler(void) __attribute__((interrupt("machine")));
void PS2_ISR(void);
void TIMER_ISR(void);

// Main body functions

// Calls the ISR for each interrupt cause
// Checks keyboard (IRQ 22) and interval timer (IRQ 0)
void handler(void) {
  int mcause_value;
  __asm__ volatile("csrr %0, mcause" : "=r"(mcause_value));

  if (mcause_value == IRQ22) {
    PS2_ISR();
  }
  if (mcause_value == TIMER_IRQ) {
    TIMER_ISR();
  }
}

void PS2_ISR(void) {
  int data;

  // When recieving break or extended codes, they come in pairs of 2 codes
  // Thus, if we recieve one of these codes we know the next code is useless
  // static - must persist between calls
  static int ignoreNext = 0;

  // Load the 16th bit - RVALID of the PS2_Data reg
  // When RVALID == 1, reading gives the data recently stored in the input FIFO
  int ps2_val;
  while ((ps2_val = *PS2_DATA) & 0x8000) {
    data = ps2_val & 0xFF;

    if (data == EXT_CODE) {
      continue;  // skip E0 prefix, still process the next byte
    }

    if (data == BREAK_CODE) {
      ignoreNext = 1;
      continue;
    }

    if (ignoreNext) {
      ignoreNext = 0;
      continue;
    }

    // Compare recieved data with each code
    // Change the value of recieved input depending on it
    switch (data) {
      case LEFT_CODE:
        currInput = LEFT;
        break;
      case RIGHT_CODE:
        currInput = RIGHT;
        break;
      case ENTER_CODE:
        currInput = ENTER;
        break;
      default:
        break;
    }
  }
}

// Timer ISR - fires every 1 second
// Counts down and sets expired flag at 0
void TIMER_ISR(void) {
  *TIMER_STATUS = 0;  // clear the timeout flag
  timer_seconds--;
  if (timer_seconds <= 0) {
    timer_seconds = 0;
    timer_expired = 1;
  }
}

void PS2_SETUP(void) {
  // When RVALID is 1, FIFO has values to read
  // Keep reading until empty
  while ((*PS2_DATA) & 0x8000) {
    int garbage = *PS2_DATA;
  }

  // Enable PS2 device level interrupts
  *PS2_CTRL = 0x1;
}

// Set interval timer to fire every 1 second
// 100 MHz clock -> period = 100,000,000 = 0x05F5E100
void TIMER_SETUP(void) {
  *TIMER_CTRL = 0x8;          // STOP timer during config
  *TIMER_PERIOD_LO = 0xE100;  // low 16 bits of 100,000,000
  *TIMER_PERIOD_HI = 0x05F5;  // high 16 bits of 100,000,000
  *TIMER_STATUS = 0;          // clear any pending timeout from a previous run
  *TIMER_CTRL = 0x7;          // START + CONT + ITO (enable interrupt)
}

// Reset timer back to TURN_TIME
void reset_timer(void) {
  *TIMER_CTRL = 0x8;          // STOP
  timer_seconds = TURN_TIME;  // reset to 10s
  timer_expired = 0;
  *TIMER_STATUS = 0;  // clear TO
  *TIMER_CTRL = 0x7;  // START + CONT + ITO
}

// Adapted largely from guide document given in quercus
void enable_interrupts(void) {
  int mstatusVal = 0b1000;
  int mtvecVal = (int)&handler;
  int mieVal;

  // disable interrupts during configuration
  __asm__ volatile("csrc mstatus, %0" ::"r"(mstatusVal));

  // set trap handler address
  __asm__ volatile("csrw mtvec, %0" ::"r"(mtvecVal));

  // clear currently enabled interrupts
  __asm__ volatile("csrr %0, mie" : "=r"(mieVal));
  __asm__ volatile("csrc mie, %0" ::"r"(mieVal));
  mieVal = (1 << 22) | (1 << 16);

  // enable PS/2 (bit 22) + Timer (bit 16) in mie
  __asm__ volatile("csrs mie, %0" ::"r"(mieVal));

  // enable processor level interrupts
  __asm__ volatile("csrs mstatus, %0" ::"r"(mstatusVal));
}

//--------------------------------------------------VGA.C-------------------------------------------------------//
#define VGA_PIXEL_BUFFER 0xC8000000
#define VGA_CTRL_PTR 0xFF203020
#define PS2_BASE 0xFF200100

#define SCREEN_W 320
#define SCREEN_H 240

#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED 0xF800
#define BLUE 0x001F
#define CYAN 0x07FF
#define YELLOW 0xFFE0

#define ROWS 6
#define COLS 7
#define EMPTY 0
#define PLAYER1 1
#define PLAYER2 2
#define CURSOR_START_COL 3

#define CELL_SIZE 28
#define CELL_GAP 2
#define CIRCLE_R 12
#define BOARD_X 54
#define BOARD_Y 30

int board[ROWS][COLS];
// int current_col;
int pixel_buffer_start;
int extended_key;
int break_code;

// 5x7 bitmap font - space (0x20) through '9' (0x39)
static const unsigned char font5x7[][7] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // space
    {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04},  // !
    {0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00},  // "
    {0x0A, 0x1F, 0x0A, 0x0A, 0x1F, 0x0A, 0x00},  // #
    {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04},  // $
    {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03},  // %
    {0x08, 0x14, 0x14, 0x08, 0x15, 0x12, 0x0D},  // &
    {0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00},  // '
    {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02},  // (
    {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08},  // )
    {0x04, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x04},  // *
    {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00},  // +
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08},  // ,
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00},  // -
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04},  // .
    {0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10},  // /
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},  // 0
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},  // 1
    {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F},  // 2
    {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E},  // 3
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},  // 4
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},  // 5
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},  // 6
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},  // 7
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},  // 8
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x11, 0x0E},  // 9
};

void plot_pixel(int x, int y, short int color) {
  if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
  volatile short int* vga =
      (volatile short int*)(pixel_buffer_start + (y << 10) + (x << 1));
  *vga = color;
}

void clear_screen(void) {
  int x, y;
  for (y = 0; y < SCREEN_H; y++)
    for (x = 0; x < SCREEN_W; x++) plot_pixel(x, y, BLACK);
}

void draw_rect(int x1, int y1, int x2, int y2, short int color) {
  int x, y;
  for (y = y1; y <= y2; y++)
    for (x = x1; x <= x2; x++) plot_pixel(x, y, color);
}

void draw_circle(int cx, int cy, int r, short int color) {
  int x, y;
  for (y = -r; y <= r; y++)
    for (x = -r; x <= r; x++)
      if (x * x + y * y <= r * r) plot_pixel(cx + x, cy + y, color);
}

// color shading for beveled board
short int darken565(short int color, int amt) {
  int r = (color >> 11) & 0x1F;
  int g = (color >> 5) & 0x3F;
  int b = color & 0x1F;
  r = r > amt ? r - amt : 0;
  g = g > amt * 2 ? g - amt * 2 : 0;
  b = b > amt ? b - amt : 0;
  return (short int)((r << 11) | (g << 5) | b);
}

short int lighten565(short int color, int amt) {
  int r = (color >> 11) & 0x1F;
  int g = (color >> 5) & 0x3F;
  int b = color & 0x1F;
  r = r + amt < 32 ? r + amt : 31;
  g = g + amt * 2 < 64 ? g + amt * 2 : 63;
  b = b + amt < 32 ? b + amt : 31;
  return (short int)((r << 11) | (g << 5) | b);
}
//

void draw_char(int x, int y, char c, short int color) {
  int idx;
  if (c >= 0x20 && c <= 0x39)  // space through '9'
    idx = c - 0x20;
  else
    return;
  int row, col;
  for (row = 0; row < 7; row++) {
    unsigned char line = font5x7[idx][row];
    for (col = 0; col < 5; col++)
      if (line & (0x10 >> col)) plot_pixel(x + col, y + row, color);
  }
}

// Draw countdown timer to the right of the board
// Shows seconds left in the current player's color
void draw_timer(int seconds, short int color) {
  int tx = BOARD_X + COLS * (CELL_SIZE + CELL_GAP) + 8;
  int ty = BOARD_Y + (ROWS * (CELL_SIZE + CELL_GAP)) / 2 - 4;

  // Clear the timer area
  draw_rect(tx, ty, tx + 14, ty + 8, BLACK);

  // Draw two-digit countdown (e.g. "10", " 5")
  int tens = seconds / 10;
  int ones = seconds % 10;
  if (tens > 0) draw_char(tx, ty, '0' + tens, color);
  draw_char(tx + 7, ty, '0' + ones, color);
}

void draw_cell(int r, int c) {
  int cx = BOARD_X + c * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
  int cy = BOARD_Y + r * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
  draw_circle(cx, cy, CIRCLE_R, BLACK);
  if (board[r][c] == PLAYER1)
    draw_circle(cx, cy, CIRCLE_R - 1, RED);
  else if (board[r][c] == PLAYER2)
    draw_circle(cx, cy, CIRCLE_R - 1, YELLOW);
}

void draw_cursor(
    int col, short int color) {  // added color param for player turn indicator
  draw_rect(BOARD_X - 2, BOARD_Y - 12,
            BOARD_X + COLS * (CELL_SIZE + CELL_GAP) + 1, BOARD_Y - 3, BLACK);
  int cx = BOARD_X + col * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
  int ay = BOARD_Y - 10;
  int i, w;
  for (i = 0; i < 6; i++) {
    w = 5 - i;
    draw_rect(cx - w, ay + i, cx + w, ay + i, color);
  }
}

void draw_board(int cursorColumn,
                short int cursorColor) {  // added cursorColor param
  int r, c;

  // beveled 3D board frame ---
  int bx1 = BOARD_X - 2;
  int by1 = BOARD_Y - 2;
  int bx2 = BOARD_X + COLS * (CELL_SIZE + CELL_GAP) + 1;
  int by2 = BOARD_Y + ROWS * (CELL_SIZE + CELL_GAP) + 1;
  short int board_mid = 0x2196;  // richer medium blue
  short int board_light = lighten565(board_mid, 7);
  short int board_dark = darken565(board_mid, 6);

  draw_rect(bx1, by1, bx2, by2, board_mid);
  draw_rect(bx1, by1, bx2, by1 + 1, board_light);
  draw_rect(bx1, by1, bx1 + 1, by2, board_light);
  draw_rect(bx1, by2 - 1, bx2, by2, board_dark);
  draw_rect(bx2 - 1, by1, bx2, by2, board_dark);
  //

  for (r = 0; r < ROWS; r++)
    for (c = 0; c < COLS; c++) draw_cell(r, c);

  int num_y = BOARD_Y + ROWS * (CELL_SIZE + CELL_GAP) + 4;
  for (c = 0; c < COLS; c++) {
    int nx = BOARD_X + c * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2 - 2;
    draw_char(nx, num_y, '1' + c, WHITE);
  }

  draw_cursor(cursorColumn, cursorColor);
}

// drop animation

void delay_loop(int count) {
  volatile int d;
  for (d = 0; d < count; d++);
}

int cell_center_y(int row) {
  return BOARD_Y + row * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
}

int cell_center_x(int col) {
  return BOARD_X + col * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
}

// Draw a colored piece only within the black hole of a specific cell
void draw_piece_in_hole(int hole_row, int col, int piece_cy, int piece_r,
                        short int color) {
  // hole center
  int hcx = cell_center_x(col);
  int hcy = cell_center_y(hole_row);
  int hr = CIRCLE_R;
  int px, py;
  for (py = -hr; py <= hr; py++) {
    for (px = -hr; px <= hr; px++) {
      int sx = hcx + px;
      int sy = hcy + py;
      // inside circle check
      if (px * px + py * py > hr * hr) continue;
      int dx = sx - hcx;
      int dy = sy - piece_cy;
      // if pixel is also inside the falling pieces circle
      if (dx * dx + dy * dy > piece_r * piece_r) continue;
      plot_pixel(sx, sy, color);
    }
  }
}

// Clear a hole by redrawing it black, then redrawing any existing piece
// Uses existing draw_cell logic inline to avoid double-drawing
void clear_hole(int row, int col) {
  int hcx = cell_center_x(col);
  int hcy = cell_center_y(row);
  draw_circle(hcx, hcy, CIRCLE_R, BLACK);
  if (board[row][col] == PLAYER1)
    draw_circle(hcx, hcy, CIRCLE_R - 1, RED);
  else if (board[row][col] == PLAYER2)
    draw_circle(hcx, hcy, CIRCLE_R - 1, YELLOW);
}

// Draw animated piece through all overlapping holes in this column
void draw_anim(int col, int piece_cy, int piece_r, short int color) {
  int r;
  // loop through all rows
  for (r = 0; r < ROWS; r++) {
    int hcy = cell_center_y(r);
    int dist = piece_cy - hcy;
    if (dist < 0) dist = -dist;
    // if distance between piece center and hole center is less than piece rad
    // to circle rad
    if (dist < piece_r + CIRCLE_R) {
      draw_piece_in_hole(r, col, piece_cy, piece_r, color);
    }
  }
}

// Erase only holes that old position touched but new position does not
void erase_anim_selective(int col, int old_cy, int new_cy, int piece_r) {
  int r;
  for (r = 0; r < ROWS; r++) {
    int hcy = cell_center_y(r);  // center of row r
    int old_dist =
        old_cy - hcy;  // vert dist between old cy and this holes center
    if (old_dist < 0) old_dist = -old_dist;
    int old_touched =
        (old_dist < piece_r + CIRCLE_R);  // circles overlapped if dist between
                                          // centers is less than sum of radii
    int new_dist =
        new_cy - hcy;  // vert dist between new cy and this holes center
    if (new_dist < 0) new_dist = -new_dist;
    int new_touched = (new_dist < piece_r + CIRCLE_R);
    if (old_touched && !new_touched) {
      clear_hole(r, col);  // if not overlapping anymore clear
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

// Forward declaration for get_open_row (defined in LOGIC section)
int get_open_row(int col);

// Animate a piece falling into the given column, then commit to board
void drop_piece_animated(int col, int piece) {
  int target_row = get_open_row(col);
  if (target_row < 0) return;

  short int color = (piece == PLAYER1) ? RED : YELLOW;
  int target_y = cell_center_y(target_row);  // where piece will end
  int start_y = cell_center_y(0);            // where piece will start, top row
  int piece_r = CIRCLE_R - 1;

  // Gravity fall
  int y = start_y;
  int prev_y = start_y;
  int velocity = 0;
  int first = 1;  // first frame flag

  while (y < target_y) {
    if (!first) {  // on frames not first, call erase to clean up where piece
                   // was
      draw_anim(col, y, piece_r, color);
      erase_anim_selective(col, prev_y, y, piece_r);
    } else {
      draw_anim(col, y, piece_r, color);
    }
    delay_loop(45000);  // control animation speed

    first = 0;  // no longer first flag
    prev_y = y;
    velocity += 2;
    y += velocity;
    if (y > target_y) y = target_y;  // prevent from overshooting target
  }

  // Snap to final position and clean up
  draw_anim(col, target_y, piece_r, color);
  erase_anim_selective(col, prev_y, target_y, piece_r);
  delay_loop(30000);  // brief pause before bounce begins

  // Phase 2: Bounce animation
  // Piece bounces 3 times with decreasing height for a natural landing feel
  // bounce_height starts at ~1 cell height and shrinks each iteration
  int bounce_height = (CELL_SIZE + CELL_GAP);  // ~30px, almost one full cell
  int b;
  for (b = 0; b < 3 && bounce_height >= 4; b++) {
    int prev_offset = 0;

    // Rise - piece moves upward from target position
    int offset;
    for (offset = 2; offset <= bounce_height; offset += 2) {
      int new_y = target_y - offset;
      int old_y = target_y - prev_offset;
      draw_anim(col, new_y, piece_r, color);
      erase_anim_selective(col, old_y, new_y, piece_r);
      delay_loop(36000);
      prev_offset = offset;
    }

    // Fall back down - piece drops back to target position
    for (offset = bounce_height - 2; offset >= 0; offset -= 2) {
      int new_y = target_y - offset;
      int old_y = target_y - prev_offset;
      draw_anim(col, new_y, piece_r, color);
      erase_anim_selective(col, old_y, new_y, piece_r);
      delay_loop(36000);
      prev_offset = offset;
    }

    // Ensure piece ends exactly at target_y after each bounce
    draw_anim(col, target_y, piece_r, color);
    erase_anim_selective(col, target_y - prev_offset, target_y, piece_r);

    // Each bounce is ~40% the height of the previous one
    bounce_height = bounce_height * 2 / 5;  // ~30 -> 12 -> 4
  }

  // Final cleanup of all holes pieces touched
  erase_anim_all(col, target_y, piece_r);

  // Commit piece to board and draw final piece using existing draw_cell
  board[target_row][col] = piece;
  draw_cell(target_row, col);
}

//--------------------------------------------------LOGIC.C-------------------------------------------------------//

int inBoundRow(int row);
int inBoundCol(int col);

// Returns if the column can have a piece there
int is_valid_col(int col) {
  return col >= 0 && col < COLS && board[0][col] == EMPTY;
}

// Gets the first open row in a column
//-1 of no open row
int get_open_row(int col) {
  int r;
  for (r = ROWS - 1; r >= 0; r--)
    if (board[r][col] == EMPTY) return r;
  return -1;
}

// Places a piece in board array
// NOTE: implement a safeguard in gameloop
// Dont let input even occur if not valid
// Make sound or display somewhere its not valid
// Return row where placed
int place_piece(int col, int colour) {
  if (is_valid_col(col)) {
    int row = get_open_row(col);
    board[row][col] = colour;
    draw_cell(row, col);  // draws colour based on board colour
    return row;
  }
  return -1;  // wont ever reach this but for safety
}

int winDetected(int rowInput, int colInput, int colour) {
  // go 4 in all directions from piece and check if theres any 4 in a rows
  // Make sure that no moves exceed board bounds - if statement lock
  // Keep running tally in that direction

  int currRow = rowInput;
  int currCol = colInput;
  int counter;

  for (int dr = -1; dr <= 1; dr++) {
    for (int dc = -1; dc <= 1; dc++) {
      // Skip duplicate directions accounted for by 2 while loops
      if (!dr && !dc) continue;
      if (dr == 1) continue;

      counter = 1;

      // Walk positive direction
      currRow = rowInput + dr;
      currCol = colInput + dc;
      while (inBoundRow(currRow) && inBoundCol(currCol) &&
             (board[currRow][currCol] == colour)) {
        currRow += dr;
        currCol += dc;
        counter++;
      }

      // Walk negative direction
      currRow = rowInput - dr;
      currCol = colInput - dc;
      while (inBoundRow(currRow) && inBoundCol(currCol) &&
             (board[currRow][currCol] == colour)) {
        currRow -= dr;
        currCol -= dc;
        counter++;
      }

      if (counter >= 4) return 1;
    }
  }

  return 0;
}

int inBoundRow(int row) { return ((row >= 0) && (row < ROWS)); }

int inBoundCol(int col) { return ((col >= 0) && (col < COLS)); }

// less resource internsive
#define DRAW_DETECTED(fullBoard, win) fullBoard && !win

int boardFull() {
  // Only need to check top row
  for (int c = 0; c < COLS; c++) {
    if (board[0][c] == 0) {
      return 0;
    }
  }

  return 1;
}

void resetBoard() {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      board[r][c] = 0;
    }
  }
}

//--------------------------------------------------MAIN.C-------------------------------------------------------//

int main() {
  volatile int* pixel_ctrl_ptr = (int*)VGA_CTRL_PTR;
  pixel_buffer_start = *pixel_ctrl_ptr;
  PS2_SETUP();
  TIMER_SETUP();
  enable_interrupts();

  runGame();
}

void runGame() {
  // Acquire keyboard input
  kbInputKey kbUserInput, prevInput;
  prevInput = NONE;

  int currentPlayer = PLAYER1;
  int currentColumn = CURSOR_START_COL;
  int lastDrawnSeconds = -1;  // tracks last displayed value to avoid flicker

  clear_screen();
  draw_board(currentColumn, RED);
  reset_timer();

  while (1) {
    __asm__ volatile("csrc mstatus, %0" ::"r"(0b1000));  // disable interrupts
    kbUserInput = currInput;
    currInput = NONE;
    int sec = timer_seconds;
    int expired = timer_expired;
    __asm__ volatile("csrs mstatus, %0" ::"r"(0b1000));  // re-enable interrupts

    // Pulse all inputs, ensuring they dont cause issues by persisting accross
    // clock cycles

    short int cursorColor = (currentPlayer == PLAYER1) ? RED : YELLOW;

    // Only redraw timer when the value changes
    if (sec != lastDrawnSeconds) {
      draw_timer(sec, cursorColor);
      lastDrawnSeconds = sec;
    }

    // Timer ran out - skip this players turn
    if (expired) {
      currentPlayer = (currentPlayer == PLAYER1) ? PLAYER2 : PLAYER1;
      cursorColor = (currentPlayer == PLAYER1) ? RED : YELLOW;
      draw_cursor(currentColumn, cursorColor);
      reset_timer();
      lastDrawnSeconds = -1;  // so new 10 gets drawn immediately
      prevInput = kbUserInput;
      continue;
    }

    if (kbUserInput == LEFT && prevInput != LEFT && currentColumn > 0) {
      currentColumn--;
      draw_cursor(currentColumn, cursorColor);
    }
    if (kbUserInput == RIGHT && prevInput != RIGHT &&
        currentColumn < COLS - 1) {
      currentColumn++;
      draw_cursor(currentColumn, cursorColor);
    }

    if (kbUserInput == ENTER && prevInput != ENTER &&
        is_valid_col(currentColumn)) {
      drop_piece_animated(currentColumn, currentPlayer);

      // Find the row where the piece landed (topmost non-empty in this column)
      int currRow;
      for (currRow = 0; currRow < ROWS; currRow++) {
        if (board[currRow][currentColumn] != EMPTY) break;
      }

      int winState = winDetected(currRow, currentColumn, currentPlayer);
      int drawState = DRAW_DETECTED(boardFull(), winState);

      if (winState || drawState) return;

      // Swap player and reset turn timer
      currentPlayer = (currentPlayer == PLAYER1) ? PLAYER2 : PLAYER1;
      cursorColor = (currentPlayer == PLAYER1) ? RED : YELLOW;
      draw_cursor(currentColumn, cursorColor);
      reset_timer();
      lastDrawnSeconds = -1;
    }
    prevInput = kbUserInput;
  }
}
