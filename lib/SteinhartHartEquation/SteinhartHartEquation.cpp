#include <math.h>
#include "SteinhartHartEquation.h"

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
