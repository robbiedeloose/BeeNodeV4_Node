#include "RandomNodeId.h"
#include <EEPROM.h>

void RandomNodeId::getId(byte *address) {
  // char firstByte = EEPROM.read(1);
  // Serial.print("First byte: ");
  // Serial.println(EEPROM.read(1));
  if (EEPROM.read(1) == '#') {
    // Serial.print("Node id: ");
    for (int i = 0; i < 4; i++) {
      address[i] = EEPROM.read(i + 2);
      // Serial.print(nodeId[i], HEX);
    }
    // Serial.println();
  } else {
    // Serial.println("Address not set. Generating code...");
    randomSeed(analogRead(A0));
    for (int i = 0; i < 4; i++) {
      address[i] = random(255);
      EEPROM.write(i + 2, address[i]);
      // Serial.print(nodeId[i], HEX);
    }
    // Serial.println();
    EEPROM.write(1, '#');
  }
}

void RandomNodeId::setId(byte *address){
  for (int i = 0; i < 4; i++) {
     EEPROM.write(i + 2, address[i]);
    // Serial.print(nodeId[i], HEX);
}}
