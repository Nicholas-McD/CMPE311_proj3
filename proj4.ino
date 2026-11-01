#include "myMilli.h"

// Pin Definitions 
const int LEDPin1 = 2;
const int LEDPin2 = 3;
const int BUTTON_PIN = 4; 
const int FAN_PWM_PIN = 9; 

// PWM Speed Values for Fan
const int PWM_OFF  = 0;
const int PWM_LOW  = 85;  // ~33% duty cycle
const int PWM_MED  = 170; // ~67% duty cycle
const int PWM_HIGH = 255; // 100% duty cycle

enum FanSpeedState {
  FAN_OFF,
  FAN_LOW,
  FAN_MED,
  FAN_HIGH
};

// Fan State Tracking Variables
FanSpeedState currentFanState = FAN_OFF;
bool fanDirectionUp = true; // true = ramping up towards HIGH; false = ramping down towards OFF

// Button Debounce Variables
bool lastButtonReading = HIGH;
bool buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50; // 50ms debounce threshold

// LED State Variables
bool ledState1 = LOW;
bool ledState2 = LOW;

// LED Intervals in msec 
unsigned long ledInterval1 = 0;
unsigned long ledInterval2 = 0;

// Non-blocking Timing Tracker Variables
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
void taskButtonAndFan();

taskFunction functionTable[]{
  taskIO,
  taskUpdateLED,
  taskButtonAndFan
};

const int NUMTASKS = sizeof(functionTable) / sizeof(functionTable[0]);

// Sets the default value for the state machine
SerialInputState serialState = PROMPT_LED;
int selectedLED = 0;

// Function declarations
void updateLEDs(unsigned long currentTime);
void handleSerialStateMachine();
void clearSerialBuffer();
void advanceFanState();
void applyFanSpeed();

void setup() {
  // Initialize custom Timer0 hardware setup replacing native millis()
  milliSetUp();

  // LED pin configurations
  pinMode(LEDPin1, OUTPUT);
  pinMode(LEDPin2, OUTPUT);
  digitalWrite(LEDPin1, LOW);
  digitalWrite(LEDPin2, LOW);

  // Button and Fan pin configurations
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(FAN_PWM_PIN, OUTPUT);
  analogWrite(FAN_PWM_PIN, PWM_OFF); // Ensure fan starts OFF

  Serial.begin(9600);
}

void loop() {
  for(int i = 0; i < NUMTASKS; i++){
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

void taskButtonAndFan() {
  bool reading = digitalRead(BUTTON_PIN);
  unsigned long currentTime = newMilli();

  // If button switch noise occurred, reset debounce timer
  if (reading != lastButtonReading) {
    lastDebounceTime = currentTime;
  }

  if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
    // If state changed after debounce delay
    if (reading != buttonState) {
      buttonState = reading;

      // Trigger state change on button press (active LOW)
      if (buttonState == LOW) {
        advanceFanState();
      }
    }
  }

  lastButtonReading = reading;
}

// Advances fan state through sequence: OFF -> LOW -> MED -> HIGH -> MED -> LOW -> OFF
void advanceFanState() {
  switch (currentFanState) {
    case FAN_OFF:
      currentFanState = FAN_LOW;
      fanDirectionUp = true;
      break;

    case FAN_LOW:
      if (fanDirectionUp) {
        currentFanState = FAN_MED;
      } else {
        currentFanState = FAN_OFF;
      }
      break;

    case FAN_MED:
      if (fanDirectionUp) {
        currentFanState = FAN_HIGH;
      } else {
        currentFanState = FAN_LOW;
      }
      break;

    case FAN_HIGH:
      currentFanState = FAN_MED;
      fanDirectionUp = false; // Reverse direction towards OFF
      break;
  }

  applyFanSpeed();
}

// Applies PWM signal to fan pin based on current state
void applyFanSpeed() {
  switch (currentFanState) {
    case FAN_OFF:
      analogWrite(FAN_PWM_PIN, PWM_OFF);
      break;
    case FAN_LOW:
      analogWrite(FAN_PWM_PIN, PWM_LOW);
      break;
    case FAN_MED:
      analogWrite(FAN_PWM_PIN, PWM_MED);
      break;
    case FAN_HIGH:
      analogWrite(FAN_PWM_PIN, PWM_HIGH);
      break;
  }
}

// Controls LED toggling without delays
void updateLEDs(unsigned long currentTime) {
  if (ledInterval1 > 0) {
    unsigned long halfInterval1 = ledInterval1 / 2;
    if (currentTime - previousTime1 >= halfInterval1) {
      previousTime1 = currentTime;
      ledState1 = !ledState1;
      digitalWrite(LEDPin1, ledState1);
    }
  } else {
    ledState1 = LOW;
    digitalWrite(LEDPin1, LOW);
  }

  if (ledInterval2 > 0) {
    unsigned long halfInterval2 = ledInterval2 / 2;
    if (currentTime - previousTime2 >= halfInterval2) {
      previousTime2 = currentTime;
      ledState2 = !ledState2;
      digitalWrite(LEDPin2, ledState2);
    }
  } else {
    ledState2 = LOW;
    digitalWrite(LEDPin2, LOW);
  }
}

void handleSerialStateMachine() {
  switch (serialState) {
    case PROMPT_LED:
      Serial.println("What LED? (1 or 2)");
      serialState = WAIT_LED;
      break;

    case WAIT_LED:
      if (Serial.available() > 0) {
        int val = Serial.parseInt(); 
        if (val == 1 || val == 2) {
          selectedLED = val;
          clearSerialBuffer(); 
          serialState = PROMPT_INTERVAL;
        } else {
          clearSerialBuffer();
          serialState = PROMPT_LED;
        }
      }
      break;

    case PROMPT_INTERVAL:
      Serial.println("What interval (in msec)?");
      serialState = WAIT_INTERVAL;
      break;

    case WAIT_INTERVAL:
      if (Serial.available() > 0) {
        int interval = Serial.parseInt();
        
        if (interval > 0) {
          if (selectedLED == 1) {
            ledInterval1 = (unsigned long)interval;
          } else if (selectedLED == 2) {
            ledInterval2 = (unsigned long)interval;
          }
          
          clearSerialBuffer(); 
          serialState = PROMPT_LED; 
        } else {
          clearSerialBuffer();
          serialState = PROMPT_INTERVAL; 
        }
      }
      break;
  }
}

void clearSerialBuffer() {
  while (Serial.available() > 0) {
    char c = Serial.peek(); 
    if (c == '\n' || c == '\r' || c == ' ') {
      Serial.read(); 
    } else {
      break;
    }
  }
}