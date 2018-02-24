#include <Arduino.h>
#include <math.h>
#include <LiquidCrystal.h>

//User variables:
const int loopEvery = 400;  //<-- set time to read the value every (in mili seconds)
const int gasValue = 800; // <-- set this when to clear the air
const unsigned long int fanManualWorkTime = 8000;       // set manual fan working time
const unsigned long int fanAutomaticWorkTime = 500;    // set automatic fan working time

//machines variables:
const int sensorCO2Pin = A0;
const int sensorCH4Pin = A1;
const int sensorTempPin = A3;
float sensorTempVin = 5; //Volts
float sensorTempLoadResistance = 10000; //Ohms
const int relay1Pin = 7;
const int relay2Pin = 8;
const int buttonLCDPin = 3;
const int buttonFanPin = 2;
///calculation variables:
int sensorCO2Read;
int sensorCH4Read;
float sensorTempRead;
float sensorTempValue;
String previouseSensorCO2Value;
String previouseSensorCH4Value;
String previouseSensorTempValue;
//state variables:
volatile byte LCDState = HIGH;
volatile byte fanState = LOW;
// time calculation variables:
unsigned long int currentMillis = 0;
unsigned long int loopEveryPreviouseMillis = 0;
unsigned long int delayByMillisPreviouse = 0;

//LCD declaration section
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 10, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//**initialize to count the Steinhart–Hart equations coefficients:
class SteinhartHartEquation {
  public:
     float Kelvin = 273.15;
     SteinhartHartEquation(float _T1, float _R1, float _T2, float _R2, float _T3, float _R3);
     float getTempKelvin(float R);
     float getTempCelsius(float R);
  private:
    float R1, R2, R3;
    float T1, T2, T3;
    float L1, L2, L3;
    float Y1, Y2, Y3;
    float y2, y3;
    float C, B, A;
};
SteinhartHartEquation::SteinhartHartEquation(float _T1, float _R1, float _T2, float _R2, float _T3, float _R3) {
    R1 = _R1;
    R2 = _R2;
    R3 = _R3;
    T1 = _T1+Kelvin;
    T2 = _T2+Kelvin;
    T3 = _T3+Kelvin;
    L1 = log(R1);
    L2 = log(R2);
    L3 = log(R3);
    Y1 = 1/T1;
    Y2 = 1/T2;
    Y3 = 1/T3;
    // y2 is gamma_2, Y2 is upsilon_2
    y2 = (Y2-Y1)/(L2-L1);
    y3 = (Y3-Y1)/(L3-L1);
    C = ((y3-y2)/(L3-L2))*(pow((L1+L2+L3),-1));
    B = y2-C*(pow(L1,2)+L1*L2+pow(L2,2));
    A = Y1-(B+(pow(L1,2))*C)*L1;
};
float SteinhartHartEquation::getTempKelvin(float R){
  float oneByT = A + B*(log(R)) + C*(pow(log(R), 3));
  float T = pow(oneByT, -1);
  return T;
}
float SteinhartHartEquation::getTempCelsius(float R){
  float oneByT = A + B*(log(R)) + C*(pow(log(R), 3));
  float T = pow(oneByT, -1);
  return T - Kelvin;
}
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
  Serial.begin(9600);
  pinMode(sensorCO2Pin, INPUT);
  pinMode(sensorCH4Pin, INPUT);
  pinMode(sensorTempPin, INPUT);
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  //momentary buttons
  pinMode(buttonLCDPin, INPUT);
  pinMode(buttonFanPin, INPUT);
  //set initial output pins state
  digitalWrite(relay1Pin, LOW);
  digitalWrite(relay2Pin, LOW);
  //button debounce declaration section using interrupts (for Arduino UNO R3 only!)
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

  turnOnOffLCD();

  if (fanState) {handleFan();}

  if (isLoopTime()) {
    sensorCO2Read = analogRead(sensorCO2Pin);
    sensorCH4Read = analogRead(sensorCH4Pin);
    sensorTempRead = analogRead(sensorTempPin);
    float sensorTempResistance = getResistance(sensorTempRead, sensorTempVin, sensorTempLoadResistance);
    sensorTempValue = sensorTemp.getTempCelsius(sensorTempResistance);

    if (LCDState) {
      clearValuesOnLCD();
    }
    if (LCDState) {
      printValuesOnLCD();
    }
    printValuesOnSerial();

    if (sensorCO2Read > gasValue || sensorCH4Read > gasValue) {
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
  Serial.println(sensorCO2Read, DEC);
  Serial.print("CH4: ");
  Serial.println(sensorCH4Read, DEC);
  Serial.print("temp: ");
  String strg = String(sensorTempValue, 1) + (char) 0xB0 + "C";
  Serial.println(strg);
}

void printValuesOnLCD() {
  //print CO2 sensor value on LCD
    lcd.setCursor(0, 0);
    lcd.print("CO2:");
    lcd.setCursor(4, 0);
    lcd.print(sensorCO2Read);
  //print CH4 sensor value on LCD
    lcd.setCursor(0, 1);
    lcd.print("CH4:");
    lcd.setCursor(4, 1);
    lcd.print(sensorCH4Read);
  //print "temp" value on serial
    lcd.setCursor(10, 0);
    lcd.print("temp:");
    lcd.setCursor(10, 1);
    String strg = String(sensorTempValue, 1) + (char)178 + "C";
    lcd.print(strg);
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
      lcd.clear();
    }
}

float getResistance(float ADCread, float Vin, float R_load) {
  float Vout = (ADCread * Vin)/1024;
  return R_load * ((Vin/Vout)-1);
};
