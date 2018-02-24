#include <Arduino.h>
#include <math.h>

const int sensorTempPin = A3;
float sensorTempVin = 5; //Voltage of the microcontoroller
float sensorTempLoadResistance = 10000; //Ohms
float sensorTempRead;
float sensorTempValue;
float sensorTempResistance;

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

void printValuesOnSerial();
float getResistance(float ADCread, float Vin, float R_load);

void setup() {
  Serial.begin(9600);
  pinMode(sensorTempPin, INPUT);
}

void loop() {
    sensorTempRead = analogRead(sensorTempPin);
    sensorTempResistance = getResistance(sensorTempRead, sensorTempVin, sensorTempLoadResistance);
    sensorTempValue = sensorTemp.getTempCelsius(sensorTempResistance);

    printValuesOnSerial();
     delay(400);
}  //main loop

//other functions
void printValuesOnSerial() {
  Serial.print("tempADCRead: ");
  Serial.print(sensorTempRead, DEC);
  Serial.println(";");
  Serial.print("temp: ");
  String strg = String(sensorTempValue, 1) + (char) 0xB0 + "C";
  Serial.println(strg);
}

float getResistance(float ADCread, float Vin, float R_load) {
  float Vout = (ADCread * Vin)/1024;
  return R_load * ((Vin/Vout)-1);
};
