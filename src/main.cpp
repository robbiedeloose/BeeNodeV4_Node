#include <arduino.h>

#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTDEC(x) Serial.print(x, DEC)
#define DEBUG_PRINTHEX(x) Serial.print(x, HEX)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTLN(x)
#endif

#include "SparkFunHTU21D.h"
#include <Wire.h>
// Create an instance of the object
HTU21D sensors;

#include "BeeNode.h"

#include <RF24.h> //Library for nRF24L01, using version https://github.com/TMRh20/RF24
#include <RF24Network.h> //Library for networking nRF24L01s, using version https://github.com/TMRh20/RF24Network
#include <SPI.h>         //nRF24L01 uses SPI communication

// Pins
#define ledPin 6 // might change to buzzerPin
#define configButton 4
#define radioPin1 7
#define radioPin2 8
#define addressPin1 A1
#define addressPin2 A2
#define addressPin3 A3
#define alarmPin 3
#define seedRef A0

// node specific declarations //
// NodeData_t beeNodeData; --------------------
uint8_t nodeAddress = 1;
uint8_t nodeMode = 1;

// EEPROM address locations //
#include <EEPROM.h>      //EEPROM functions
#define EEPRomDeviceId 1 // 1 byte for #, 4 bytes for ID
#define EEPRomOptions 6
#define EEPRomDs18Ids 16 // 48 bytes for 6 addresses, 8 bytes each
uint8_t nodeId[4];

// own libraries //
#include "RandomNodeId.h"
RandomNodeId beeNodeId;

#include "MesureVoltageInternal.h"
float iREF = 1.1;
MesureVoltageInternal battery(iREF);

// Low Power
#include <LowPower.h>
uint8_t sleepInterval = 10;
int sleepcycle = 2;

// RF24 declarations //
RF24 radio(7, 8); // create object to control and communicate with nRF24L01
RF24Network network(radio); // Create object to use nRF24L01 in mesh network
const uint16_t rXNode = 00; // address of coordinator

// structure used to hold the payload that is sent to the coordinator.
struct payload_t {
  uint8_t id[4];
  int16_t temp;
  uint16_t bat;
  uint16_t weight;
  uint16_t humidity;
};
payload_t payload; // Payload to send

// RF25 Radio CODE
// /////////////////////////////////////////////////////////////////////
void collectData(payload_t *payloadAddress) {
  payloadAddress->bat = battery.getVoltage() * 100;
  payloadAddress->weight = 8888;
  payloadAddress->humidity = sensors.readHumidity() * 10;
  payloadAddress->temp = sensors.readTemperature() * 100;
}

void showData(payload_t *payloadAddress) {
  Serial.print("Device Id: "); // Show devide id
  for (uint8_t i = 0; i < 4; i++)
    Serial.print(nodeId[i], HEX);
  Serial.println();
  Serial.print("Device Address: ");
  Serial.println(nodeAddress); // Show device address
  Serial.print("Temperature: ");
  Serial.println((float)payloadAddress->temp / 100);
  Serial.print("Humidity: ");
  Serial.println((float)payloadAddress->humidity / 10);
  Serial.print("Battery: ");
  Serial.println((float)payloadAddress->bat / 100); // Show battery info
  Serial.println();
}

// should edit it to accept pointer to struct as well, probably ad it with & in function header en remove the & within the function
bool sendData(payload_t *payloadAddress, int sizeOfPayload) { 
  // check to see if there is any network traffic that needs to be passed on, technically an end device does not need this
  network.update(); 
  // Create transmit header. This goes in transmit packet to help route it where it needs to go, in this case it is the coordinator
  RF24NetworkHeader header(rXNode); 
  // send data onto network and make sure it gets there1
  if (network.write(header, payloadAddress, sizeOfPayload))
    return true;
  else
    return false;
}

// NODE ADDRESS from pin headers ///////////////////////////////////////////////
uint8_t getNodeAdress() {
  pinMode(addressPin1, INPUT_PULLUP);
  pinMode(addressPin2, INPUT_PULLUP);
  pinMode(addressPin3, INPUT_PULLUP);
  uint8_t address = 0;
  if (digitalRead(addressPin1) == LOW) {
    address = address + 1;
  }
  if (digitalRead(addressPin2) == LOW) {
    address = address + 2;
  }
  if (digitalRead(addressPin3) == LOW) {
    address = address + 3;
  }
  if (address == 0)
    address = 1;
  return address;
}

// INIT FUNCTIONS //////////////////////////////////////////////////////////////
void initNode() {
  // Set pinmodes for config mode
  pinMode(ledPin, OUTPUT);
  // Get ID and Address
  beeNodeId.getId(nodeId); // send array to fill as parameter
  nodeAddress = getNodeAdress();
  // Set voltage reference
  battery.setRefInternal();
  // set sleepcycle
  sleepcycle = sleepInterval * 60 / 8;
  // unit sensors
  sensors.begin();
}

void initRadio() { //
  SPI.begin();     // Start SPI communication
  radio.begin();   // start nRF24L01 communication and control
  radio.setPALevel(HIGH);
  // radio.setRetries(0,0);
  // setup network communication, first argument is channel which determines
  // frequency band module communicates on. Second argument is address of this
  // module
  network.begin(90, nodeAddress);
}

bool checkResponse() {
  if (Serial.available() > 0) {
    char receivedChar = Serial.read();
    if (receivedChar == 'y') {
      // Serial.println("YES");
      return true;
    } else {
      // Serial.println("NO");
      return false;
    }
  }
  return false;
}

bool getAnswer() {
  while (!Serial.available()) {
  }
  if (Serial.read() == 'y')
    return true;
  else
    return false;
}

void saveSettings() {
  DEBUG_PRINTLN("saving settings");
  EEPROM.put(EEPRomOptions, 0x88);
  EEPROM.put(EEPRomOptions + 1, 0);
  EEPROM.put(EEPRomOptions + 2, sleepInterval);
  EEPROM.put(EEPRomOptions + 3, 0);
  EEPROM.put(EEPRomOptions + 4, iREF);
  EEPROM.put(EEPRomOptions + 8, nodeMode);
}
void loadSettings() {
  DEBUG_PRINTLN(EEPROM.read(EEPRomOptions));
  if (EEPROM.read(EEPRomOptions) != 0xFF) {
    DEBUG_PRINTLN("loading settings");
    delay(5000);
    EEPROM.get(EEPRomOptions + 2, sleepInterval);
    EEPROM.get(EEPRomOptions + 4, iREF);
    EEPROM.get(EEPRomOptions + 8, nodeMode);
  } else {
    DEBUG_PRINTLN("settings not saved");
  }
}

void getNewSettings() {
  //// Set DeviceId ///////////////
  /////////////////////////////////
  Serial.println(F("Change DeviceAddress? y/n"));
  if (getAnswer()) {
    // if we pressed YES
    Serial.println(F("Enter address per byte in decimal format: "));
    byte id[4];
    for (int i = 0; i < 4; i++) {
      while (!Serial.available())
        ;
      int val = Serial.parseInt();
      id[i] = char(val);
      Serial.println("next byte please");
    }
    Serial.print("new address: ");
    for (byte b : id)
      Serial.print(b, HEX);
    Serial.println();
    Serial.println(F("Is this correct? y/n"));
    if (getAnswer())
      // save address to eeprom
      beeNodeId.setId(id);
  } else {
    // other key is pressed
  }

  //// Set number of sensors ////////
  ///////////////////////////////////
  Serial.println(F("Change number of sensors? y/n"));
  if (getAnswer()) {
    // if we pressed YES
    Serial.println(F("Enter 3 or 6: Not possible now"));
    while (!Serial.available()) {
    }
    int tSensors = Serial.parseInt();
    if (tSensors == 3 || tSensors == 6)
      //numberOfSensors = (uint8_t)tSensors;
    ;
  } else {
    // other key is pressed
  }

  //// set Interval
  Serial.println(F("Change interval? y/n"));
  if (getAnswer()) {
    // if we pressed YES
    Serial.println(F("Enter interval in minutes (max 240)"));
    while (!Serial.available()) {
    }
    int tInterv = Serial.parseInt();
    if (tInterv >= 1 && tInterv <= 240)
      sleepInterval = (byte)tInterv;

  } else {
  }

  //// set vref
  Serial.println(F("Change Vref? y/n"));
  if (getAnswer()) {
    // if we pressed YES
    Serial.println(F("Give vref between 1.0 and 1.2"));
    while (!Serial.available()) {
    }
    float ref = Serial.parseFloat();
    if (ref >= 1.0 && ref <= 1.2)
      iREF = ref;
  } else {
    // other key is pressed
    Serial.println(F("Out of range - ignored"));
  }

  Serial.println(F("Change Mode? y/n"));
  if (getAnswer()) {
    Serial.println(F("1: node, 2: router + node, 3: router"));
    while (!Serial.available()) {
      int tMode = Serial.parseInt();
      if (tMode > 0 && tMode < 4)
        nodeMode = (uint8_t)tMode;
    }
  } else {
    // other key is pressed
  }
  saveSettings();
  initNode();
}
////////////////////////////////////////////////////////////////////////////////
void printConfig() {
  Serial.println("BeeNode v3.0.1a Node");
  Serial.println("-----------------------------------------");
  Serial.print("Node Id: ");
  for (byte b : nodeId)
    Serial.print(b, HEX);
  Serial.println();
  Serial.print("Node Address: ");
  Serial.println(nodeAddress);
  Serial.println("-----------------------------------------");
  Serial.print("Vref: ");
  Serial.println(iREF);
  Serial.println("-----------------------------------------");
  Serial.print("Interval: ");
  Serial.print(sleepInterval);
  Serial.print(" (");
  Serial.print(sleepcycle);
  Serial.println(" cycles of 8 seconds)");
  Serial.println("-----------------------------------------");
  Serial.println("Humidity sensor");
  Serial.println("Present: false");
  Serial.println("-----------------------------------------");
  Serial.println("Scale sensor");
  Serial.println("Present: false");
  Serial.println("-----------------------------------------");
  Serial.print("Change setting? y/n");
  for (int i = 0; i < 20; i++) {
    Serial.print(".");
    if (checkResponse() == true) {
      getNewSettings();
      break;
    }
    delay(500);
  }
  Serial.println();
  Serial.println();
}

////////////////////////////////////////////////////////////////////////////////
// SETUP AND LOOP
// //////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  #ifdef DEBUG
    while (!Serial) {
      ; // wait for serial port not to miss data when starting serial monitor
      // leave out when debug is not defined
    }
    #endif
  loadSettings(); // load settings from eeprom
  initNode(); // start node
  initRadio(); // start radio
  printConfig(); // print settings data
}

void loop() {
  // fill struct
  collectData(&payload);
  #ifdef DEBUG
    // display data in struct
    showData(&payload);
    #endif
  // send struct
  bool sendSuccesfull = sendData(&payload, sizeof(payload));
  // display send result
  #ifdef DEBUG
    if (sendSuccesfull)
      DEBUG_PRINTLN("Send succesfull");
    else
      DEBUG_PRINTLN("Send error!");
    DEBUG_PRINTLN();
    #endif
  delay(1000);
  // power down radio for 8s x sleepcycle
  radio.powerDown();
  for (int i; i < sleepcycle; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  radio.powerUp();
}
