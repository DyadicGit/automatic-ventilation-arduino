#include <math.h>
#ifndef SteinhartHartEquation_h
#define SteinhartHartEquation_h

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
#endif
