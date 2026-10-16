#ifndef MYMILLI_H
#define MYMILLI_H

#include <Arduino.h>
#include <avr/interrupt.h>


//defines each of the registers that is used during this program
//the registers are voltile since they can change during runtime without insturction formt he program
#define COUNTERREG       (*(volatile unsigned char *)0x46) 
#define STATUSREG        (*(volatile unsigned char *)0x5F) 
#define COUNTSTOPREG    (*(volatile unsigned char *)0x47) 
#define INTERRUPTMASKREG (*(volatile unsigned char *)0x6E) 
#define CTRLREG_A        (*(volatile unsigned char *)0x44) 
#define CTRLREG_B        (*(volatile unsigned char *)0x45) 

// Bit positioning constants, used to set the correct bits during setup
#define PRESCALERBIT_A 1 
#define PRESCALERBIT_B 0 
#define COMPARECLEAR   1 
#define COMPAREENABLE  1 

//iterrated varible which keeps track of the amount of milliseconds that have passed
static volatile unsigned long millis_counter = 0;

//iterrated the millisec counter varible by one after each interrupt
ISR(TIMER0_COMPA_vect) {
  millis_counter++;
}

//setsup the registers tot he correct conditions 
inline void milliSetUp() {
  // Set CTC mode
  CTRLREG_A = (1 << COMPARECLEAR);           

  // Set prescaler to 64
  CTRLREG_B = (1 << PRESCALERBIT_A) | (1 << PRESCALERBIT_B);

  //This register is set to 249 so that when the 
  //counter count to 250 (249, is becasue we include 0 in this), exactly 1 ms has passed
  COUNTSTOPREG = 249;                                

  //rests the counter back to 0
  COUNTERREG  = 0;                                 
  
  //enables the intrupt so that the status flag register can be notified when the timer has finished
  INTERRUPTMASKREG = (1 << COMPAREENABLE);                 
  
  //global status interrupt 
  STATUSREG |= 0x80;                                 
}

// Replacement for native millis()
inline unsigned long newMilli() {
  unsigned long currentMillis;
  unsigned char oldStatusREG = STATUSREG;

  STATUSREG &= ~(0x80);
  currentMillis = millis_counter;
  
  // Restores interrupt flag state
  STATUSREG = oldStatusREG; 

  return currentMillis;
}

#endif