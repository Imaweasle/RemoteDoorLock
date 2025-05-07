/* 
 * Project Remote Doorlock Central
 * Author: Tyler Schneider, Alexander Ung
 * Date: 4/28
*/

#include "Particle.h"

SYSTEM_MODE(SEMI_AUTOMATIC); // Semi-Auto to control when to connect to the cloud, we only want to connect after BLE is connected
//SYSTEM_THREAD(ENABLED);

bool cloudConnecting = false;

SerialLogHandler logHandler(LOG_LEVEL_INFO);

// Number of services to grab in each BLE scan
const size_t scanResultsCount = 30;
BleScanResult scanResults[scanResultsCount];

BlePeerDevice peer; // Sets up the Central BLE device
// Configures service and characteristics
BleCharacteristic notifyChar; 
BleCharacteristic responseChar;
BleCharacteristic manualChar;
const BleUuid serviceUuid("1AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5"); // BLE doorlock service
const BleUuid notifyCharUuid("2AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5"); // BLE notification characteristic (scanner -> central)
const BleUuid responseCharUuid("3AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5"); // BLE response characteristic (central -> scanner)
const BleUuid manualCharUuid("4AD18FF7-2A5A-44D7-B0A6-A87BA00A50E5"); // BLE manual lock/freeze characteristic (central -> scanner)

//led
const int ledPin = D4;

//door lock variables
bool doorUnlocked = false;
String doorState = "locked";
String prevDoorState = "locked";

//manual lock/freeze variables
String manualState = "unlocked";
String prevManualState = "unlocked";

// Motor pins
const int PH = D1;
const int EN = A5;
int curPH = LOW;
int speed = 0;
bool motorRunning = false;

bool locking = false;
bool unlocking = false;

// Button pins
const int lockButton = D2;
const int unLockButton = D3;
int lockButtonState = LOW;
int prevLockButtonState = LOW;
int unLockButtonState = LOW;
int prevUnLockButtonState = LOW;

// Limit switch pins
const int limitSwitch = D6;  // Lock side limit switch
const int limitSwitch2 = D7; // Unlock side limit switch

/*
void doorStringtoBool() {
  if (doorState == "locked") {
    doorUnlocked = false;
  }
  else if (doorState == "unlocked") {
    doorUnlocked = true;
  }
}
*/

// A lot of functions, so we declare them here
void bleCloudConnect();
void handleData(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

int forceDoorState(String command1);
int forceManualState(String command2);

void checkButtonPresses();
void motorControl();

void sendStateChanges();
bool doorStateChanged();
bool manualStateChanged();

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);

  // BLE stuff
  BLE.on(); // initialize bluetooth
  notifyChar.onDataReceived(handleData, &notifyChar); // Calls the handleData function when data is received from the notify characteristic

  // Setup pins
  pinMode(PH, OUTPUT);
  pinMode(EN, OUTPUT);
  pinMode(lockButton, INPUT);
  pinMode(unLockButton, INPUT);
  pinMode(limitSwitch, INPUT_PULLUP);  // Using internal pullup
  pinMode(limitSwitch2, INPUT_PULLUP); // Using internal pullup
  
  // Check if at limit when powered on
  if (digitalRead(limitSwitch) == HIGH || digitalRead(limitSwitch2) == HIGH) {
    motorRunning = false;
  }

  // Set cloud variables
  Particle.variable("cV_doorState", doorState);
  Particle.variable("cV_manualState", manualState);
  Particle.function("cF_forceDoorState", forceDoorState);
  Particle.function("cF_forceManualState", forceManualState);
}

void loop() {
  bleCloudConnect(); // Checks and connects BLE, then connects to Cloud

  checkButtonPresses();

  motorControl();

  sendStateChanges();

  // this if-else exists for demonstration purposes, to clearly see if the door is locked or unlocked
  if (doorState == "unlocked" && !motorRunning) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);  
  }

  delay(25);
}

void bleCloudConnect() {
  // This function connects this central photon to the scanner photon via BLE, and then connects this central photon to the cloud

  // If the photon isn't already connected to a BLE peripheral
  if (!peer.connected()) {
    Log.info("Scanning...");
    int scanCount = BLE.scan(scanResults, scanResultsCount); // Scan for up to 30 peripherals at a time

    for (int i = 0; i < scanCount; i++) {
      BleUuid foundServiceUuid;
      size_t svcCount = scanResults[i].advertisingData().serviceUUID(&foundServiceUuid, 1); // Check if any of the service UUIDS match what we're scanning for

      if (svcCount > 0 && foundServiceUuid == serviceUuid) { // Connects to the service UUID if found
        Log.info("Found service. connecting...");
        peer = BLE.connect(scanResults[i].address());

        if (peer.connected()) { 
          Log.info("Connected to peripheral");
          delay(500);

          peer.discoverAllCharacteristics(); // If successfully connected, we search for the service's characteristics
          
          // Tries to retrieve all characteristics we're looking for from the service
          if (peer.getCharacteristicByUUID(notifyChar, notifyCharUuid) && peer.getCharacteristicByUUID(responseChar, responseCharUuid) && peer.getCharacteristicByUUID(manualChar, manualCharUuid)) {
            Log.info("Characteristics Found");
          }
          else {
            Log.info("not fucking connecting piece of shit characteristics"); // pardon my french
            peer.disconnect();
          }
        } 
      }
    }
  }
  // If BLE is connected, but cloud isn't connected, start connecting to cloud
  else if (peer.connected() && !Particle.connected() && !cloudConnecting) {
    cloudConnecting = true;
    Particle.connect();
  }
  // If cloud is connected, and we were previously trying to connect, reset the flag
  else if (Particle.connected() && cloudConnecting) {
    cloudConnecting = false;
  }
}

// This function handles BLE data coming from the scanner photon (outside of the door), getting called when data is received
// The variable "data" holds the data received and is converted into a string for use within the scanner photon
void handleData(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context) { 
  if (len > 0) {
    doorState = String((const char*)data, len); // can probably call an decryption function here if we want
    Log.info("Door status: %s", doorState.c_str());
  }
}

// This function receives and handles doorState data from the cloud
int forceDoorState(String command1) { 
  if (command1 == "locked") {
    doorState = "locked";
    Log.info("Door State Forced: %s", doorState.c_str());
    return 1;
  }
  else if (command1 == "unlocked") {
    doorState = "unlocked";
    Log.info("Door State Forced: %s", doorState.c_str());
    return 1;
  }
  else {
    Log.info("Invalid Command: %s", command1.c_str());
    return -1;
  }
}

// This function receives and handles manualState data from the cloud
int forceManualState(String command2) {
  if (command2 == "locked") {
    manualState = "locked";
    Log.info("Manual State Forced: %s", manualState.c_str());
    return 1;
  }
  else if (command2 == "unlocked") {
    manualState = "unlocked";
    Log.info("Manual State Forced: %s", manualState.c_str());
    return 1;
  }
  else {
    Log.info("Invalid Command: %s", command2.c_str());
    return -1;
  }
}

// Anything relating to the button presses - Either lock or unlock the door
void checkButtonPresses() {
  // Read current button states
  lockButtonState = digitalRead(lockButton);
  unLockButtonState = digitalRead(unLockButton); 

  // Handle button presses
  if (unLockButtonState == HIGH && prevUnLockButtonState == LOW) {
    unlocking = true;
    Serial.println("Unlock button pressed");
  }

  if (lockButtonState == HIGH && prevLockButtonState == LOW) {
    locking = true;
    Serial.println("Lock button pressed");
  }

  // Update previous button states
  prevLockButtonState = lockButtonState;
  prevUnLockButtonState = unLockButtonState;
}

void motorControl() {
  if (locking) {
    curPH = HIGH;           // Set motor direction for lock
    speed = 40;
    motorRunning = true;
  }
  else if (unlocking) {
    curPH = LOW;            // Set motor direction for unlock
    speed = 40;             // Set motor speed
    motorRunning = true;
  }

  // Check limit switches (for normally closed setup)
  if (motorRunning) {
    if (curPH == HIGH && digitalRead(limitSwitch) == HIGH) {
      Serial.println("Lock limit switch triggered - stopping motor");
      motorRunning = false;
      doorState = "locked";
      locking = false;
    }
    if (curPH == LOW && digitalRead(limitSwitch2) == HIGH) {
      Serial.println("Unlock limit switch triggered - stopping motor");
      motorRunning = false;
      doorState = "unlocked";
      unlocking = false;
    }
  }

  // Motor control
  if (motorRunning) {
    analogWrite(EN, speed); // Motor runs
  } else {
    analogWrite(EN, 0);     // Motor stopped
  }

  // Always assert correct motor direction
  digitalWrite(PH, curPH);
}

bool doorStateChanged() { // used in sendStateChanges()
  // Checks if the state of the door lock has changed, then return a bool value
  if (doorState != prevDoorState) {
    prevDoorState = doorState;

    return true;
  }
  else {
    return false;
  }
}

bool manualStateChanged() { // used in sendStateChanges()
  // Checks if the state of the manual lock has changed, then return a bool value
  if (manualState != prevManualState) {
    prevManualState = manualState;

    return true;
  }
  else {
    return false;
  }
}

void sendStateChanges() {
  // If any change was made to "doorState" we update and send data to the scanner photon & publish it to the cloud
  if (doorStateChanged()) { 
    if (peer.connected()) {
      const char* state1 = doorState;
      responseChar.setValue(state1);
      Log.info("Sent Response Value: %s", doorState.c_str());
    }
    if (doorState == "locked") {
      locking = true;
      Particle.publish("Door Locked");
    }
    else if (doorState == "unlocked") {
      unlocking = true;
      Particle.publish("Door Unlocked");
    }
  }

  // If any change was made to "manualState" we update and send data to the scanner photon & publish it to the cloud
  if (manualStateChanged()) {
    if (peer.connected()) {
      const char* state2 = manualState;
      manualChar.setValue(state2);
      Log.info("Sent Response Value: %s", manualState.c_str());
    }
    if (manualState == "unlocked") {
      Particle.publish("Door Unfrozen");
    }
    else if (manualState == "locked") {
      Particle.publish("Door Frozen");
    }
  }
}
