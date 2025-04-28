#include "Particle.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler(LOG_LEVEL_INFO);

const BleUuid serviceUuid("1AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5");
const BleUuid notifyCharUuid("2AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5");
const BleUuid responseCharUuid("3AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5");

BleCharacteristic notifyChar("doorState", BleCharacteristicProperty::NOTIFY, notifyCharUuid, serviceUuid);
BleCharacteristic responseChar("successState", BleCharacteristicProperty::WRITE_WO_RSP, responseCharUuid, serviceUuid);

bool doorUnlocked = false;
String doorState = "locked";
bool responseFunctionCalled = false;

int lastToggle = 0;
int toggleInterval = 10000;
int ledPin = D4;

int buttonPin = D3;
bool buttonState = HIGH;
bool prevButtonState = HIGH;

void handleData(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
  responseFunctionCalled = true;
  if (len > 0) {
    success = String((const char*)data, len);
    Log.info("Did it Work: %s", success.c_str());  
  }
}

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  BLE.on();
  
  BLE.addCharacteristic(notifyChar);
  BLE.addCharacteristic(responseChar);

  BleAdvertisingData adv;
  adv.appendLocalName("DoorLock");
  adv.appendServiceUUID(serviceUuid);
  BLE.advertise(&adv);

  Log.info("Started BLE Door Service (svc %s)", serviceUuid.toString().c_str());

  responseChar.onDataReceived(handleData, &responseChar);
}

void loop() {
  if (BLE.connected()) {
    buttonState = digitalRead(buttonPin);
    if (buttonState == LOW && prevButtonState == HIGH) {
      prevButtonState = LOW;

      if (doorState == "locked") {
        doorState = "unlocked";
      }
      else {
        doorState = "locked";
      }

      const char* state = doorState;
      notifyChar.setValue(state);
      Log.info("Notified door state: %s", doorState.c_str());
    }
    else if (buttonState == HIGH && prevButtonState == LOW) {
      prevButtonState = HIGH;
    }

    if (responseFunctionCalled) {
      responseFunctionCalled = false;
      if (doorState == "unlocked") {
        digitalWrite(ledPin, HIGH);  //visual test
      } 
      else if (doorState == "locked"){
        digitalWrite(ledPin, LOW);  
      }
    }
  }
  delay(10);
}
