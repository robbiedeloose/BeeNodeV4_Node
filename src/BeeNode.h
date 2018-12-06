
#ifndef BeeNode_h
#define BeeNode_h

#include <arduino.h>

typedef struct {
  uint8_t nodeId[4];
  uint8_t nodeAddress;
  uint8_t hiveTemps[3];
  uint8_t batteryVoltage;
  uint8_t alarm;
} NodeData_t;

#endif
