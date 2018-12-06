#ifndef RandomNodeId_h
#define RandomNodeId_h

#include <Arduino.h>
#include <EEPROM.h>

class RandomNodeId {
public:
  RandomNodeId() { ; }
  void getId(byte *nodeId);
  void setId(byte *nodeId);

private:
};

#endif
