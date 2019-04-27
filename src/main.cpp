#include <Arduino.h>
#include <U8x8lib.h>
#include "../lib/icons/Icons.h"
#include "../lib/SteinhartHartEquation/SteinhartHartEquation.h"

//User variables:
const int loopEvery = 500;  //<-- set time to read the value every (in miliseconds)
const int gasValueCO2 = 170; // <-- set this when to clear the air
const int gasValueCH4 = 320; // <-- set this when to clear the air
const unsigned long int fanManualWorkTime = (unsigned long int) 1000*60*0.5;       // set manual fan working time 15mim
const unsigned long int fanTurboModeWorkTime = (unsigned long int) 1000*60*0.25;  // set fan on turbo mode working time, should be less tan fanManualWorkTime!
const unsigned long int fanAutomaticWorkTime =  (unsigned long int) 1000*60*1;    // set automatic fan working time 15min

//machines variables:
const int sensorCO2Pin = A0;
const int sensorCH4Pin = A1;
const int sensorTempPin = A3;
float sensorTempVin = 5; //Volts (Voltage of the microcontoroller)
float sensorTempLoadResistance = 10000; //Ohms
const int relay1FanPin = 4;
const int relay2TurboModePin = 5;
const int ledFan1Pin = 6;
const int buttonLCDPin = 3;
const int buttonFanPin = 2;
const byte ON = LOW;
const byte OFF = HIGH;
///calculation variables:
int sensorCO2Read;
int sensorCH4Read;
float sensorTempRead;
float sensorTempValue;
String previouseSensorCO2Value;
String previouseSensorCH4Value;
String previouseSensorTempValue;
uint8_t rowCO2  = 0;
uint8_t rowCH4  = 2;
uint8_t rowTemp = 1;
//state variables:
volatile byte LCDState = HIGH;
volatile byte fanState = LOW;
// time calculation variables:
unsigned long int currentMillis = 0;
unsigned long int loopEveryPreviouseMillis = 0;
unsigned long int delayByMillisPreviouse = 0;
//LCD declaration section
U8X8_SSD1306_128X32_UNIVISION_SW_I2C u8x8(/* clock=A5*/ 19, /* data=A4*/ 18);

//**initialize to count the Steinhart–Hart equations coefficients:
//** values taken from "NTC Thermistors - Murata" datasheet page:15,
//** for a NTC MF52-series 103-type 10kOhm Thermistor (Proteus device code: NCP21XV103)
SteinhartHartEquation sensorTemp( /*T1*/5, /*R1*/25194, /*T2*/25, /*R2*/10000, /*T3*/45, /*R3*/4401);

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
void showWhoTrigerredFan(bool printIt);
float getResistance(float ADCread, float Vin, float R_load);

void setup() {
  pinMode(sensorCO2Pin, INPUT);
  pinMode(sensorCH4Pin, INPUT);
  pinMode(sensorTempPin, INPUT);
  pinMode(relay1FanPin, OUTPUT);
  pinMode(relay2TurboModePin, OUTPUT);
  pinMode(ledFan1Pin, OUTPUT);
  //momentary buttons
  pinMode(buttonLCDPin, INPUT);
  pinMode(buttonFanPin, INPUT);
  //set initial output pins state
  digitalWrite(relay1FanPin, OFF);
  digitalWrite(relay2TurboModePin, OFF);
  digitalWrite(ledFan1Pin, LOW);
  //button debounce declaration section using interrupts (for Arduino UNO R3 only!)
  attachInterrupt(digitalPinToInterrupt(buttonLCDPin), handleLCDInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(buttonFanPin), handleFanInterrupt, CHANGE);
  // set up the LCD:
  u8x8.begin();
  u8x8.setPowerSave(0);
  // Print a welcome message to the LCD.
  u8x8.setFont(u8x8_font_pxplusibmcgathin_f);
  u8x8.print("Studios 83");
  u8x8.drawTile(1, 2, 1, Icons::house);
  delay(1000);
  for(uint8_t i=10; i<13; i++) {
    u8x8.setCursor(i, 0);
    u8x8.print(".");
    delay(800);
  }
}

void loop() {
  currentMillis = millis();
  if (currentMillis < loopEvery+2027) {      // this code line is just to display that the machine works when it stats for the first time
    u8x8.drawTile(15, 4, 1, Icons::house);  // "2027" is a variable I've got when simulating (experimental principal)
  }

  turnOnOffLCD();

  if (fanState) {
      handleFan();
  }

  if (isLoopTime()) {
    sensorCO2Read = analogRead(sensorCO2Pin);
    sensorCO2Read = floor(sensorCO2Read/10)*10;
    sensorCH4Read = analogRead(sensorCH4Pin);
    sensorCH4Read = floor(sensorCH4Read/10)*10;
    sensorTempRead = analogRead(sensorTempPin);
    float sensorTempResistance = getResistance(sensorTempRead, sensorTempVin, sensorTempLoadResistance);
    sensorTempValue = sensorTemp.getTempCelsius(sensorTempResistance);

    clearValuesOnLCD();
    printValuesOnLCD();

    if (sensorCO2Read >= gasValueCO2 && sensorCH4Read >= gasValueCH4) {
      digitalWrite(relay1FanPin, ON);
      digitalWrite(relay2TurboModePin, OFF);
      digitalWrite(ledFan1Pin, HIGH);

      showWhoTrigerredFan(true);
      delayByMillisPreviouse = 0;
      int timeCounter = (int) fanAutomaticWorkTime;
      while (timeCounter>0 && !fanState) {
        turnOnOffLCD();
        if (isDelayTime(1)) {
          timeCounter--;
        }
      }
      showWhoTrigerredFan(false);
    } else {
      digitalWrite(relay1FanPin, OFF);
      digitalWrite(relay2TurboModePin, OFF);
      digitalWrite(ledFan1Pin, LOW);
    }

    previouseSensorCO2Value = (String) sensorCO2Read;
    previouseSensorCH4Value = (String) sensorCH4Read;
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
  if (interrupt_time - last_interrupt_time > 250)
    {
      toggleStateLCD();
    }
  last_interrupt_time = interrupt_time;
}
void handleFanInterrupt() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 150)
    {
      toggleStateFan();
    }
  last_interrupt_time = interrupt_time;
}

// action handling functions
void handleFan() {
  digitalWrite(relay1FanPin, ON);
  digitalWrite(relay2TurboModePin, ON);
  digitalWrite(ledFan1Pin, HIGH);

  int timeCounter = (int) round(fanManualWorkTime/1000);
  u8x8.clearDisplay();
  u8x8.setCursor(0, 0);
  u8x8.print("fan working T");
  u8x8.drawTile(12, 2, 2, Icons::ventilator16x16_1of2);
  u8x8.drawTile(12, 3, 2, Icons::ventilator16x16_2of2);
  u8x8.setCursor(1, 2);
  u8x8.print(timeCounter);
  delayByMillisPreviouse = 0;
  while (timeCounter>=0 && fanState) {
    if (isDelayTime(1000)) {
      u8x8.setCursor(1, 2);
      u8x8.print(timeCounter);
      if (timeCounter < 10000) {u8x8.print("    ");}
      timeCounter--;
    }
    if (timeCounter <= (int)round(fanTurboModeWorkTime / 1000)) {
      digitalWrite(relay2TurboModePin, OFF);
      u8x8.setCursor(0, 0);
      u8x8.print("fan working  ");
    }
    turnOnOffLCD();
  }
  u8x8.clearDisplay();
  printValuesOnLCD();
  fanState = LOW;
}

void turnOnOffLCD() {
    u8x8.setPowerSave(!LCDState);
}

void printValuesOnLCD() {
  //print CO2 sensor value on LCD
    u8x8.setCursor(0, rowCO2);
    u8x8.print("CO2:");
    u8x8.setCursor(4, rowCO2);

    u8x8.print(sensorCO2Read);
  //print CH4 sensor value on LCD
    u8x8.setCursor(0, rowCH4);
    u8x8.print("CH4:");
    u8x8.setCursor(4, rowCH4);
    u8x8.print(sensorCH4Read);
  //print "temp" value on LCD
    u8x8.setCursor(10, rowTemp);
    String strg = String(sensorTempValue, 1) + (char) 0xB0 + "C";
    u8x8.print(strg);
}

void clearValuesOnLCD() {
  //clears the screen if length of values changed
  String strSensorCO2Value = (String) sensorCO2Read;
  String strSensorCH4Value = (String) sensorCH4Read;
  String strSensorTempValue = (String) sensorTempValue;
    if (previouseSensorCO2Value.length() != strSensorCO2Value.length() ||
        previouseSensorCH4Value.length() != strSensorCH4Value.length() ||
        previouseSensorTempValue.length() != strSensorTempValue.length()
      ) {
      u8x8.clearDisplay();
    }
}

void showWhoTrigerredFan(bool printIt) {
  const int column = 8;
  if (printIt) {
    if (sensorCO2Read >= gasValueCO2) {u8x8.drawTile(column, rowCO2, 1, Icons::ventilator8x8);}
    if (sensorCH4Read >= gasValueCH4) {u8x8.drawTile(column, rowCH4, 1, Icons::ventilator8x8);}
  }
  if (!printIt) {
    u8x8.drawString(column, rowCO2, " ");
    u8x8.drawString(column, rowCH4, " ");
  }
}

float getResistance(float ADCread, float Vin, float R_load) {
  float Vout = (ADCread * Vin)/1024;
  return R_load * ((Vin/Vout)-1);
};
