#include "Particle.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

bool cloudConnecting = false;

SerialLogHandler logHandler(LOG_LEVEL_INFO);

const size_t scanResultsCount = 30;
BleScanResult scanResults[scanResultsCount];

BlePeerDevice peer;
BleCharacteristic notifyChar;
BleCharacteristic responseChar;
const BleUuid serviceUuid("1AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5");
const BleUuid notifyCharUuid("2AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5");
const BleUuid responseCharUuid("3AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5");

const int ledPin = D4;
bool doorUnlocked = false;
String doorState = "locked";
String prevDoorState = "locked";
bool doorFunctionCalled = false;

void doorStringtoBool() {
  if (doorState == "locked") {
    doorUnlocked = false;
  }
  else if (doorState == "unlocked") {
    doorUnlocked = true;
  }
}

void bleCloudConnect();

bool doorStateChanged();

void handleData(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

int forceDoorState(String command);

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  BLE.on();

  notifyChar.onDataReceived(handleData, &notifyChar);

  Particle.variable("cV_doorState", doorState);
  Particle.function("cF_forceDoorState", forceDoorState);
}

void loop() {
  bleCloudConnect();

  if (doorStateChanged()) { 
    
    if (peer.connected()) {
      const char* state = doorState;
      responseChar.setValue(state);
      Log.info("Sent Response Value: %s", doorState.c_str());
    }
    // do stuff here that unlocks the door physically
  }

  if (doorState == "unlocked") {
    digitalWrite(ledPin, HIGH);  //visual test
  } else {
    digitalWrite(ledPin, LOW);  
  }
}

void bleCloudConnect() {
  if (!peer.connected()) {
    Log.info("Scanning...");
    int scanCount = BLE.scan(scanResults, scanResultsCount);

    for (int i = 0; i < scanCount; i++) {
      BleUuid foundServiceUuid;
      size_t svcCount = scanResults[i].advertisingData().serviceUUID(&foundServiceUuid, 1);

      if (svcCount > 0 && foundServiceUuid == serviceUuid) {
        Log.info("Found service. connecting...");
        peer = BLE.connect(scanResults[i].address());

        if (peer.connected()) {
          Log.info("Connected to peripheral");
          delay(500);

          peer.discoverAllCharacteristics();
          
          if (peer.getCharacteristicByUUID(notifyChar, notifyCharUuid) && peer.getCharacteristicByUUID(responseChar, responseCharUuid)) {
            Log.info("Characteristics Found");
          }
          else {
            Log.info("not fucking connecting piece of shit characteristics");
            peer.disconnect();
          }
        } 
      }
    }
  }
  else if (peer.connected() && !Particle.connected() && !cloudConnecting) {
    cloudConnecting = true;
    Particle.connect();
  }
  else if (Particle.connected() && cloudConnecting) {
    cloudConnecting = false;
  }
}

bool doorStateChanged() {
  if (doorState != prevDoorState) {
    prevDoorState = doorState;

    if (doorState == "unlocked") {
      Particle.publish("Door Unlocked");
    }
    else if (doorState == "locked") {
      Particle.publish("Door Locked");
    }

    return true;
  }
  else {
    return false;
  }
}

void handleData(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) { // Handles data from Peripheral (outside of door)
  //doorFunctionCalled = true;
  if (len > 0) {
    doorState = String((const char*)data, len); // can probably call an decryption function here if we want
    Log.info("Door status: %s", doorState.c_str());
  }
}

int forceDoorState(String command) { // Handles data from cloud
  //doorFunctionCalled = true;
  if (command == "locked") {
    doorState = "locked";
    Log.info("Door State Forced: %s", doorState.c_str());
    return 1;
  }
  else if (command == "unlocked") {
    doorState = "unlocked";
    Log.info("Door State Forced: %s", doorState.c_str());
    return 1;
  }
  else {
    Log.info("Invalid Command: %s", command.c_str());
    return -1;
  }
}