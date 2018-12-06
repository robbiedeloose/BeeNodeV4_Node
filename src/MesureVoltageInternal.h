#ifndef MesureVoltageInternal_h
#define MesureVoltageInternal_h

#include <Arduino.h>

class MesureVoltageInternal {
public:
  MesureVoltageInternal(float iREF);
  float getVoltage();
  void setRefInternal();

private:
  void burn8Readings();
  float _iREF;
};

#endif
