// CPULator Requires all code in 1 file and address macros explicitly declared

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

// Define internal alias for keyboard input
typedef enum { NONE, LEFT, RIGHT, ENTER } kbInputKey;
volatile kbInputKey currInput = NONE;

// Function definitions

// Required definition for interrupt handler
// Only handler for all interrupts
// Will need to readjust code when we add more interrupts potentially
static void handler(void) __attribute__((interrupt("machine")));
void PS2_ISR(void);

// Main body functions

// This function is adapated from the guide
// Calls the ISR for each cause
// For now just keyboard
void handler(void) {
  int mcause_value;
  __asm__ volatile("csrr %0, mcause" : "=r"(mcause_value));

  if (mcause_value == IRQ22) {
    PS2_ISR();
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
  while ((*PS2_DATA) & 0x8000) {
    // Delete any residual data in Data reg by reading
    data = *PS2_DATA & 0xFF;

    if ((data == BREAK_CODE) || (data == EXT_CODE)) {
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

void PS2_SETUP(void) {
  // When RVALID is 1, FIFO has values to read
  // Keep reading until empty
  while ((*PS2_DATA) & 0x8000) {
    int garbage = *PS2_DATA;
  }

  // Enable PS2 device level interrupts
  *PS2_CTRL = 0x1;
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
  mieVal = (1 << 22);

  // enable PS/2 IRQ 22 = bit 22 of mie for keyboard
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
#define YELLOW 0xFF0F

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

// 5x7 bitmap font - only chars we need (0-7, space, and a few letters)
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
      if (line & (0x10 >> col)) plot_pixel(x + col, y + row, color);
  }
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

void draw_cursor(int col) {
  draw_rect(BOARD_X - 2, BOARD_Y - 12,
            BOARD_X + COLS * (CELL_SIZE + CELL_GAP) + 1, BOARD_Y - 3, BLACK);
  int cx = BOARD_X + col * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2;
  int ay = BOARD_Y - 10;
  int i, w;
  for (i = 0; i < 6; i++) {
    w = 5 - i;
    draw_rect(cx - w, ay + i, cx + w, ay + i, RED);
  }
}

void draw_board(int cursorColumn) {
  int r, c;
  draw_rect(BOARD_X - 2, BOARD_Y - 2,
            BOARD_X + COLS * (CELL_SIZE + CELL_GAP) + 1,
            BOARD_Y + ROWS * (CELL_SIZE + CELL_GAP) + 1, BLUE);

  for (r = 0; r < ROWS; r++)
    for (c = 0; c < COLS; c++) draw_cell(r, c);

  int num_y = BOARD_Y + ROWS * (CELL_SIZE + CELL_GAP) + 4;
  for (c = 0; c < COLS; c++) {
    int nx = BOARD_X + c * (CELL_SIZE + CELL_GAP) + CELL_SIZE / 2 - 2;
    draw_char(nx, num_y, '1' + c, WHITE);
  }

  draw_cursor(cursorColumn);
}

//--------------------------------------------------LOGIC.C-------------------------------------------------------//

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
  enable_interrupts();

  runGame();
}

void runGame() {
  // Acquire keyboard input
  kbInputKey kbUserInput, prevInput;
  prevInput = NONE;

  int currentPlayer = PLAYER1;
  int currentColumn = CURSOR_START_COL;

  draw_board(currentColumn);

  while (1) {
    // Constantly track and update cursor column
    kbUserInput = currInput;
    currInput = NONE;
    

    //Pulse all inputs, ensuring they dont cause issues by persisting accross clock cycles


    if (kbUserInput == LEFT && prevInput != LEFT && currentColumn > 0) {
      currentColumn--;
      draw_cursor(currentColumn);
      currInput = NONE;
    }
    if (kbUserInput == RIGHT && prevInput != RIGHT && currentColumn < COLS - 1) {
      currentColumn++;
      draw_cursor(currentColumn);
      currInput = NONE;
    }

    if (kbUserInput == ENTER && prevInput != ENTER && is_valid_col(currentColumn)) {
      int currRow = place_piece(currentColumn, currentPlayer);
      draw_board(currentColumn);
      currInput = NONE;

      int winState = winDetected(currRow, currentColumn, currentPlayer);
      int drawState = DRAW_DETECTED(boardFull(), winState);

      if (winState || drawState) return;

      // Swap player
      currentPlayer = (currentPlayer == PLAYER1) ? PLAYER2 : PLAYER1;
    }
    prevInput = kbUserInput;
  }
}
