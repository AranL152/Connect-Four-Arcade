#include "address_map.h"

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

// testing process
/*
int main(void) {
  PS2_SETUP();
  enable_interrupts();
  volatile int* LEDR = LEDR_BASE;

  while (1) {
    kbInputKey kbUserInput = currInput;
    currInput = NONE;

    if (kbUserInput == LEFT) {
      *LEDR_DEST = 0b0000000001;  // LEDR0
    }
    if (kbUserInput == RIGHT) {
      *LEDR_DEST = 0b0000000010;  // LEDR1
    }
    if (kbUserInput == ENTER) {
      *LEDR_DEST = 0b0000000100;  // LEDR2
    }
  }
}
*/