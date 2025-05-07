/* 
 * Project Remote Doorlock Scanner
 * Author: Lydia Schieffer, Alexander Ung
 * Date: 4/28
*/

// Include Particle Device OS APIs
#include "Particle.h"
#include "NFC.h"
#include "LCDDEFS.h"
#include "LiquidCrystal_I2C.h"

SYSTEM_MODE(SEMI_AUTOMATIC); // We don't want the scanner to be connected, only the central, so we never connect this photon to the cloud

// Run the application and system concurrently in separate threads
//SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// Defines the UUIDS for BLE
const BleUuid serviceUuid("1AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5"); // BLE doorlock service
const BleUuid notifyCharUuid("2AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5"); // BLE notification characteristic (scanner -> central)
const BleUuid responseCharUuid("3AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5"); // BLE response characteristic (central -> scanner)
const BleUuid manualCharUuid("4AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5"); // BLE manual lock/freeze characteristic (central -> scanner)

// Defines the characteristics and type for BLE
BleCharacteristic notifyChar("doorState", BleCharacteristicProperty::NOTIFY, notifyCharUuid, serviceUuid); // send value
BleCharacteristic responseChar("successState", BleCharacteristicProperty::WRITE_WO_RSP, responseCharUuid, serviceUuid); // receive value
BleCharacteristic manualChar("manualState", BleCharacteristicProperty::WRITE_WO_RSP, manualCharUuid, serviceUuid); // receive value

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool correctUID;
uint8_t *uid = nullptr;
const uint8_t goodUID[7] = {0x04, 0x4C, 0x14, 0x29, 0xB7, 0x2A, 0x81}; 

String doorState = "locked";
bool responseFunctionCalled = false;

String manualState = "unlocked";
bool manualLock = false;
bool manualFunctionCalled = false;

// This function handles BLE data coming from the central photon, getting called when data is received
// The variable "data" holds the data received and is converted into a string for use within the scanner photon
void handleDataResponse(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
  responseFunctionCalled = true;
  if (len > 0) {
    doorState = String((const char*)data, len); // Convert to string
    Log.info("Did it work???? %s", doorState.c_str());
  }
}

// This function handles BLE data coming from the central photon, getting called when data is received
// The variable "data" holds the data received and is converted into a string for use within the scanner photon
void handleDataManual(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) {
  manualFunctionCalled = true; // prompts scanner to check if it should be blocked or unblocked
  if (len > 0) {
    manualState = String((const char*)data, len); // Convert to string
    Log.info("Did it work???? %s", manualState.c_str());  
  }
  // Converts manualState into a bool
  if (manualState == "unlocked") {
    manualLock = false;
  }
  else if (manualState == "locked") {
    manualLock = true;
  }
}

NFCAuth nfc;

void setup() {
  // BLE setup
  BLE.on();
  
  // adds the BLE characteristics to the photon
  BLE.addCharacteristic(notifyChar);
  BLE.addCharacteristic(responseChar);
  BLE.addCharacteristic(manualChar);

  // Sets up BLE advertising information
  BleAdvertisingData adv;
  adv.appendLocalName("DoorLock");
  adv.appendServiceUUID(serviceUuid);
  BLE.advertise(&adv); // advertises

  Log.info("Started BLE Door Service (svc %s)", serviceUuid.toString().c_str());

  // Tells the respective handleData functions to be called when data is received for each characteristic
  responseChar.onDataReceived(handleDataResponse, &responseChar);
  manualChar.onDataReceived(handleDataManual, &manualChar);

  delay(1000);
  nfc.setup();

  //Setup LCD
  lcd.init();
  lcd.backlight();
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

  if (BLE.connected()) { // Only works when the two photons are connected

    if(!manualLock) {
      manualFunctionCalled = false;

      if(!correctUID){
        lcd.clear();
        lcd.print(PROMPT_FOR_CARD);
        uid = nfc.scan(); //get UID
      }
      if(uid != nullptr) {
        correctUID = true;
        for(int i = 0; i < 7; i++) { //7 bit UID
          if(uid[i]!= goodUID[i]) {
            correctUID = false;
          }
        }
        //Grant access
        if(correctUID) {
          lcd.clear();
          lcd.print(ACCEPTED);

          doorState = "unlocked";
          
          // Sends doorState = unlocked to central photon
          const char* state = doorState;
          notifyChar.setValue(state);
          Log.info("Notified door state: %s", doorState.c_str());

          while(uid != nullptr) {
            uid = nfc.scan();
            //Particle.process();
            delay(200);
          }
          correctUID = false;
          
        }
        //Deny access 
        else {
          lcd.clear();
          lcd.print(DENIED);

          doorState = "locked";
          
          // Sends doorState = locked to central photon
          const char* state = doorState;
          notifyChar.setValue(state);
          Log.info("Notified door state: %s", doorState.c_str());

          while(uid != nullptr) {
            uid = nfc.scan();
            //Particle.process();
            delay(200);
          }
        }
      }
    } 
    else if(manualLock && manualFunctionCalled) {
      lcd.clear();
      lcd.print(LOCKED);
      manualFunctionCalled = false;
    }
  }
}



