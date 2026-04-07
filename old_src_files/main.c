#include "address_map.h"
#include "kb_decoder.c"
#include "vga.c"

int main() {
  volatile int* pixel_ctrl_ptr = (int*)VGA_CTRL_PTR;
  pixel_buffer_start = *pixel_ctrl_ptr;
  PS2_SETUP();
  enable_interrupts();

  runGame();
  resetBoard();

}

void runGame() {
  // Acquire keyboard input
  kbInputKey kbUserInput, prevInput;

  //Pulse all inputs, ensuring they dont cause issues 
  //by persisting accross clock cycles
  prevInput = NONE;

  int currentPlayer = PLAYER1;
  int currentColumn = CURSOR_START_COL;

  draw_board(currentColumn);

  while (1) {
    // Constantly track and update cursor column
    kbUserInput = currInput;
    currInput = NONE;
    
    if (kbUserInput == LEFT && prevInput != LEFT && currentColumn > 0) {
      currentColumn--;
      draw_cursor(currentColumn);
      currInput = NONE;
    }
    
    if (kbUserInput == RIGHT && prevInput != RIGHT && currentColumn < (COLS - 1)) {
      currentColumn++;
      draw_cursor(currentColumn);
      currInput = NONE;
    }

    //Only accept user input if on valid column
    if (kbUserInput == ENTER && prevInput != ENTER && is_valid_col(currentColumn)) {
      //Place piece, save the row
      int currRow = place_piece(currentColumn, currentPlayer);

      //Redraw board, reset keyboard input
      draw_board(currentColumn);
      currInput = NONE;

      //Check if game has ended
      int winState = winDetected(currRow, currentColumn, currentPlayer);
      int drawState = DRAW_DETECTED(boardFull(), winState);

      if (winState || drawState) return;

      // Swap player
      currentPlayer = (currentPlayer == PLAYER1) ? PLAYER2 : PLAYER1;
    }

    //Save this cycles input as prev input
    prevInput = kbUserInput;
  }
}
