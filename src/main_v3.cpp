#include <Arduino.h>
#include <LiquidCrystal.h>
//User values
const int loopEvery = 400;  //<-- set time to read the value every (in mili seconds)
const int gasValue = 800; // <-- set this when to clear the air
const unsigned long int fanManualWorkTime = 8000;       // set manual fan working time
const unsigned long int fanAutomaticWorkTime = 500;    // set automatic fan working time
///machine value declaration ////
int sensorCO2Value;
int sensorCH4Value;
int sensorTempValue;
//previouse values
String previouseSensorCO2Value;
String previouseSensorCH4Value;
String previouseSensorTempValue;
//
int sensorCO2Pin = 0;
int sensorCH4Pin = 1;
int sensorTempPin = 6;
int relay1Pin = 7;
int relay2Pin = 8;
// time calculation variables
unsigned long int currentMillis = 0;
unsigned long int loopEveryPreviouseMillis = 0;
unsigned long int delayByMillisPreviouse = 0;
//button debounce declaration section using interrupts (for Arduino UNO!)
const int buttonLCDPin = 3;
const int buttonFanPin = 2;
volatile byte LCDState = HIGH;         // the initial state of the output pin
volatile byte fanState = LOW;         // the initial state of the output pin
//LCD declaration section
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 10, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//function declaration section
void toggleStateLCD();
void handleLCDInterrupt();
void handleFanInterrupt();
void toggleStateFan();
void handleFan();
bool isLoopTime();
bool isDelayTime(unsigned long int delayByMillis);
void printValuesOnLCD();
void clearValuesOnLCD();
void printValuesOnSerial();
void turnOnOffLCD();

void setup() {
  Serial.begin(9600);      // sets the serial port to 9600
  pinMode(sensorCO2Pin, INPUT);
  pinMode(sensorCH4Pin, INPUT);
  pinMode(sensorTempPin, INPUT);
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  //momentary buttons
  pinMode(buttonLCDPin, INPUT);  //LCD on/off momentary button
  pinMode(buttonFanPin, INPUT);  //fan on button
  //set initial output pins state
  digitalWrite(relay1Pin, LOW);
  digitalWrite(relay2Pin, LOW);

  attachInterrupt(digitalPinToInterrupt(buttonLCDPin), handleLCDInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(buttonFanPin), handleFanInterrupt, CHANGE);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a welcome message to the LCD.
  lcd.print("Studios83");
  delay(500);
  for(uint8_t i=9; i<12; i++) {
    lcd.setCursor(i, 0);
    lcd.print(".");
    delay(500);
  }
}

void loop() {
  currentMillis = millis();
  if (currentMillis < loopEvery+2027) {lcd.blink();} else {lcd.noBlink();}  // this code line is just to display that the machine works when it stats for the first time
                                                                          // "2027" is a variable I've got when simulating (experimental principal)
  turnOnOffLCD();

  if (fanState) {handleFan();}

  if (isLoopTime()) {
    // read values from sensors:
    sensorCO2Value = analogRead(sensorCO2Pin);
    sensorCH4Value = analogRead(sensorCH4Pin);
    sensorTempValue = analogRead(sensorTempPin);

    if (LCDState) {
      clearValuesOnLCD();
    }
    if (LCDState) {
      printValuesOnLCD();
    }
    printValuesOnSerial();

    if (sensorCO2Value > gasValue || sensorCH4Value > gasValue) {
      digitalWrite(relay1Pin, HIGH);
      digitalWrite(relay2Pin, HIGH);

      if (LCDState) { lcd.setCursor(15, 1); }
      delayByMillisPreviouse = 0;
      int timeCounter = fanAutomaticWorkTime;
      while (timeCounter>0 && !fanState) {
        if (LCDState) { lcd.blink(); }  //shows that fan is ON
        turnOnOffLCD();
        if (isDelayTime(1)) {
          timeCounter--;
        }
      }
      if (LCDState) { lcd.noBlink(); }
    } else {
      digitalWrite(relay1Pin, LOW);
      digitalWrite(relay2Pin, LOW);
    }

    previouseSensorCO2Value = (String) sensorCO2Value;
    previouseSensorCH4Value = (String) sensorCH4Value;
    previouseSensorTempValue = (String) sensorTempValue;
  } //if isLoopTime();
}  //main loop

//time calculation functions
bool isLoopTime() {
  bool result = currentMillis - loopEveryPreviouseMillis >= loopEvery;
  if (result) {loopEveryPreviouseMillis = currentMillis;}
  return result;
}
bool isDelayTime(unsigned long int delayByMillis) {
  currentMillis = millis();
  bool result = (currentMillis - delayByMillisPreviouse >= delayByMillis) || (currentMillis < delayByMillis);
  if (result) {delayByMillisPreviouse = currentMillis;}
  return result;
}

//state toggle functions
void toggleStateLCD() {
  LCDState = !LCDState;
}
void toggleStateFan() {
  fanState = !fanState;
}

void handleLCDInterrupt() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If button press come faster than 50ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 50)
    {
      toggleStateLCD();
    }
  last_interrupt_time = interrupt_time;
}
void handleFanInterrupt() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 50)
    {
      toggleStateFan();
    }
  last_interrupt_time = interrupt_time;
}

// action handling functions
void handleFan() {
  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);

  int timeCounter = (int) round(fanManualWorkTime/1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("fan working");
  lcd.setCursor(0, 1);
  lcd.print(timeCounter);
  delayByMillisPreviouse = 0;
  while (timeCounter>=0 && fanState) {
    if (isDelayTime(1000)) {
      if (LCDState) {
        lcd.setCursor(0, 1);
        lcd.print(timeCounter);
        if (timeCounter < 10000) {lcd.print("    ");}
      }
      timeCounter--;
    }
    turnOnOffLCD();
  }
  lcd.clear();
  printValuesOnLCD();
  fanState = LOW;
}

void turnOnOffLCD() {    //used also to handle LCD toggling not in main loop
  if (LCDState) {
    lcd.display();
  } else {
    lcd.noDisplay();
  }
}

//other functions
void printValuesOnSerial() {
  Serial.print("CO2: ");
  Serial.println(sensorCO2Value, DEC);
  Serial.print("CH4: ");
  Serial.println(sensorCH4Value, DEC);
  Serial.print("temp: ");
  Serial.print(sensorTempValue*5/1024, DEC);
  Serial.println("*C");
}

void printValuesOnLCD() {
  //print CO2 sensor value on LCD
    lcd.setCursor(0, 0);
    lcd.print("CO2:");
    lcd.setCursor(4, 0);
    lcd.print(sensorCO2Value);
  //print CH4 sensor value on LCD
    lcd.setCursor(0, 1);
    lcd.print("CH4:");
    lcd.setCursor(4, 1);
    lcd.print(sensorCH4Value);
  //print "temp" value on serial
    lcd.setCursor(10, 0);
    lcd.print("temp:");
    lcd.setCursor(10, 1);
    lcd.print(sensorTempValue*5/1024);
    lcd.print((char)178); //degree symbol
    lcd.print("C");
}

void clearValuesOnLCD() {
  //clears the screen if length of values changed
  String strSensorCO2Value = (String) sensorCO2Value;
  String strSensorCH4Value = (String) sensorCH4Value;
  String strSensorTempValue = (String) sensorTempValue;
    if (previouseSensorCO2Value.length() != strSensorCO2Value.length() ||
        previouseSensorCH4Value.length() != strSensorCH4Value.length() ||
        previouseSensorTempValue.length() != strSensorTempValue.length()
      ) {
      lcd.clear();
    }
}
