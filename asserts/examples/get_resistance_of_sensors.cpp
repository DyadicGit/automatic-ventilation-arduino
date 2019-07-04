#include <Arduino.h>

//machines variables:
const int sensorTempPin = A3;
float sensorTempVin = 5;                //Volts (Voltage of the microcontoroller)
float sensorTempLoadResistance = 10000; //Ohms

//function declarations
float getResistance(float ADCread, float Vin, float R_load);

void setup()
{
    pinMode(sensorTempPin, INPUT);
}

void loop()
{
    float sensorTempResistance = getResistance(
            analogRead(sensorTempPin),
            sensorTempVin,
            sensorTempLoadResistance
        );
}

float getResistance(float ADCread, float Vin, float R_load)
{
    float Vout = (ADCread * Vin) / 1024;
    return R_load * ((Vin / Vout) - 1);
};