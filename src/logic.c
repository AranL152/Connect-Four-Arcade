#include "vga.c"

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
  // Initialize starting row and col
  int currRow = rowInput;
  int currCol = colInput;
  int counter;

  // Loop through all 8 directions on board
  for (int dr = -1; dr <= 1; dr++) {
    for (int dc = -1; dc <= 1; dc++) {
      // Skip duplicate directions accounted for by using 2 while loops
      if (!dr && !dc) continue;
      if (dr == 1) continue;

      counter = 1;

      // Walk positive direction
      // Ensure bounds arent exceeded
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

// Based on if game has been won and if board is full
//  less resource internsive to use a macro
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
