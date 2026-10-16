#include "myMilli.h"

// Pin Definitions 
const int LEDPin1 = 2;
const int LEDPin2 = 3;

// LED State Variables
bool ledState1 = LOW;
bool ledState2 = LOW;

// LED Intervals in msec 
unsigned long ledInterval1 = 0;
unsigned long ledInterval2 = 0;

// Non-blocking Timing Tracker Variables (unsigned long required for millis())
unsigned long previousTime1 = 0;
unsigned long previousTime2 = 0;

// State machine states for non-blocking serial communication
enum SerialInputState {
  PROMPT_LED,
  WAIT_LED,
  PROMPT_INTERVAL,
  WAIT_INTERVAL
};

typedef void (*taskFunction)();

void taskUpdateLED();
void taskIO();

taskFunction functionTable[]{
  taskIO,
  taskUpdateLED
};


const int NUMTASKS = sizeof(functionTable) / sizeof(functionTable[0]);

// Sets the default value for the state machine
SerialInputState serialState = PROMPT_LED;
int selectedLED = 0;

void setup() {
  // Defines the LEDs to pins 2 and 3 respectively 
  milliSetUp();

  pinMode(LEDPin1, OUTPUT);
  pinMode(LEDPin2, OUTPUT);

  digitalWrite(LEDPin1, LOW);
  digitalWrite(LEDPin2, LOW);

  Serial.begin(9600);

  //testIntialCondition();//test to make sure that the intial conditions are correct  
}

void loop() {
  for(int i = 0; i<NUMTASKS; i++){
    functionTable[i]();
  }
}

void taskUpdateLED(){
  unsigned long time = newMilli();
  updateLEDs(time);
}

void taskIO(){
  handleSerialStateMachine();
}

// Controls LED toggling without delays
void updateLEDs(unsigned long currentTime) {
  // LED 1 Logic
  // Checks if the LED interval is positive, if not then the LED is set to LOW
  if (ledInterval1 > 0) {
    // Checks if half the time has passed, if it has then reset the timer
    unsigned long halfInterval1 = ledInterval1 / 2;
    if (currentTime - previousTime1 >= halfInterval1) {
      previousTime1 = currentTime;
      // Flip the LED state to the opposite state it is currently in
      ledState1 = !ledState1;
      digitalWrite(LEDPin1, ledState1);
      
      //printTime(currentTime, 1, ledState1);//test to make sure that the LEDs activate on the intervals that they are supposed to 

    }
  } else {
    ledState1 = LOW;
    digitalWrite(LEDPin1, LOW);
  }

  // LED 2 Logic
  // Checks if the LED interval is positive, if not then the LED is set to LOW
  if (ledInterval2 > 0) {
    // Checks if half the time has passed, if it has then reset the timer
    unsigned long halfInterval2 = ledInterval2 / 2;
    if (currentTime - previousTime2 >= halfInterval2) {
      // Resets the timer
      previousTime2 = currentTime;
      // Flip the LED state to the opposite state it is currently in
      ledState2 = !ledState2;
      digitalWrite(LEDPin2, ledState2);
      //printTime(currentTime, 2, ledState2);//test to make sure that the LEDs activate on the intervals that they are supposed to

    }
  } else {
    ledState2 = LOW;
    digitalWrite(LEDPin2, LOW);
  }
}

// Checks the states of the input. Checks if there is an input in the serial buffer, 
// if there is then input is processed, if not then the code is skipped so the rest of the program can run without waiting 
void handleSerialStateMachine() {
  switch (serialState) {

    // First case asks for the LED then waits
    case PROMPT_LED:
      Serial.println("What LED? (1 or 2)");
      serialState = WAIT_LED;
      break;

    // If an input is given, the value is saved and the state machine moves to prompt interval
    case WAIT_LED:
      if (Serial.available() > 0) {
        int val = Serial.parseInt(); 
        if (val == 1 || val == 2) {
          selectedLED = val;
          
          // Clears the buffer so that trailing newlines aren't reread later
          clearSerialBuffer(); 
          
          serialState = PROMPT_INTERVAL;
        } else {
          // If the input was invalid, clear buffer and prompt again
          clearSerialBuffer();
          serialState = PROMPT_LED;
        }
      }
      break;

    // Prompts the user for an interval, and waits 
    case PROMPT_INTERVAL:
      Serial.println("What interval (in msec)?");
      serialState = WAIT_INTERVAL;
      break;

    // If an input is given, the value is saved and the state machine loops back
    case WAIT_INTERVAL:
      if (Serial.available() > 0) {
        int interval = Serial.parseInt();
        
        // Only accept intervals greater than 0
        if (interval > 0) {

          // Sets the interval to the correct LED
          if (selectedLED == 1) {
            ledInterval1 = (unsigned long)interval;
          } else if (selectedLED == 2) {
            ledInterval2 = (unsigned long)interval;
          }
          
          // Clears the buffer so that trailing newlines aren't reread later
          clearSerialBuffer(); 
          
          serialState = PROMPT_LED; // Loop back for next interaction pair
        } else {
          clearSerialBuffer();
          serialState = PROMPT_INTERVAL; // Retry interval if zero or invalid
        }
      }
      break;
  }
}

// Helper function to discard trailing whitespace, \r, and \n
void clearSerialBuffer() {
  // Checks if there is something on the serial buffer
  while (Serial.available() > 0) {

    // Check next character without removing it
    char c = Serial.peek(); 
    if (c == '\n' || c == '\r' || c == ' ') {
      // Discard the newline or space
      Serial.read(); 
    } else {
      break;
    }
  }
}



//                    ***TESTER FUNCTIONS***



//tests to ensure that the program is working correctly
/*
void testIntialCondition(){
    Serial.println("\nInital conditions at the time of startup");
    Serial.print("LED 1 pin: ");
    Serial.println(LEDPin1);
    Serial.print("LED 2 pin: ");
    Serial.println(LEDPin2);
    Serial.print("LED 1 inital state: ");
    Serial.println(ledState1);
    Serial.print("LED 2 inital state: ");
    Serial.println(ledState2);
    Serial.print("LED 1 inital interval: ");
    Serial.println(ledInterval1);
    Serial.print("LED 2 inital interval: ");
    Serial.println(ledInterval1);
}
void printTime(unsigned long time, int LED, bool state){
    Serial.print("LED:");
    Serial.println(LED);
    Serial.print("State: ");
    Serial.println(state);
    Serial.print("Time of activation (in ms): ");
    Serial.println(time);
}
*/

/*
void testMyMilli(){
    if(myMilli != millis()){
        serial.println("Timers dotn match!");
    }
}
/*