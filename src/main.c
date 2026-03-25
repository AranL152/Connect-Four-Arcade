#include "address_map.h"
#include "kb_decoder.c"
#include "vga.c"

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
