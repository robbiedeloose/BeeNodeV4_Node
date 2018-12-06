#include "MesureVoltageInternal.h"
#include <arduino.h>

MesureVoltageInternal::MesureVoltageInternal(float iREF) { _iREF = iREF; }

// This function uses the known internal reference value of the 328p (~1.1V) to
// calculate the VCC value which comes from a battery
// This was leveraged from a great tutorial found at
// https://code.google.com/p/tinkerit/wiki/SecretVoltmeter?pageId=110412607001051797704
float MesureVoltageInternal::getVoltage() {
  analogReference(EXTERNAL); // set the ADC reference to AVCC
  burn8Readings(); // make 8 readings but don't use them to ensure good reading
                   // after ADC reference change
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  unsigned long start = millis(); // get timer value
  while ((start + 3) > millis())
    ;                  // delay for 3 milliseconds
  ADCSRA |= _BV(ADSC); // Start ADC conversion
  while (bit_is_set(ADCSRA, ADSC))
    ;                  // wait until conversion is complete
  int result = ADCL;   // get first half of result
  result |= ADCH << 8; // get rest of the result
  float batVolt = (_iREF / result) *
                  1024;      // Use the known iRef to calculate battery voltage
  analogReference(INTERNAL); // set the ADC reference back to internal
  burn8Readings(); // make 8 readings but don't use them to ensure good reading
                   // after ADC reference change
  return batVolt;
}

// This function makes 8 ADC measurements but does nothing with them
// Since after a reference change the ADC can return bad readings. This function
// is used to get rid of the first
// 8 readings to ensure next reading is accurate
void MesureVoltageInternal::burn8Readings() {
  for (int i = 0; i < 8; i++) {
    analogRead(A0);
    analogRead(A1);
  }
}

void MesureVoltageInternal::setRefInternal() {
  analogReference(INTERNAL); // set the ADC reference to internal 1.1V reference
  burn8Readings(); // make 8 readings but don't use them to ensure good reading after ADC reference change
}
