#ifndef MYMILLI_H
#define MYMILLI_H

#include <Arduino.h>
#include <avr/interrupt.h>

// Registers used during this program
#define COUNTERREG       (*(volatile unsigned char *)0x46) 
#define STATUSREG        (*(volatile unsigned char *)0x5F) 
#define COUNTSTOPREG    (*(volatile unsigned char *)0x47) 
#define INTERRUPTMASKREG (*(volatile unsigned char *)0x6E) 
#define CTRLREG_A        (*(volatile unsigned char *)0x44) 
#define CTRLREG_B        (*(volatile unsigned char *)0x45) 

// Bit positioning constants
#define PRESCALERBIT_A 1 
#define PRESCALERBIT_B 0 
#define COMPARECLEAR   1 
#define COMPAREENABLE  1 

static volatile unsigned long millis_counter = 0;

ISR(TIMER0_COMPA_vect) {
  millis_counter++;
}

inline void milliSetUp() {
  CTRLREG_A        = (1 << COMPARECLEAR);                         // Set CTC mode
  CTRLREG_B        = (1 << PRESCALERBIT_A) | (1 << PRESCALERBIT_B); // Set prescaler to 64
  COUNTSTOPREG    = 249;                                         // 1ms match count
  COUNTERREG      = 0;                                           // Reset timer counter
  INTERRUPTMASKREG = (1 << COMPAREENABLE);                        // Enable match interrupt
  STATUSREG       |= 0x80;                                        // Enable global interrupts
}

inline unsigned long newMilli() {
  unsigned long currentMillis;
  unsigned char oldStatusREG = STATUSREG;

  STATUSREG &= ~(0x80); // Atomically disable interrupts
  currentMillis = millis_counter;
  STATUSREG = oldStatusREG; // Restore interrupt state

  return currentMillis;
}

#endif // MYMILLI_H