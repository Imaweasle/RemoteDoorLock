
#include "NFC.h"
NFCAuth::NFCAuth() {
    pn532i2c = new PN532_I2C(Wire);
    nfc = new PN532(*pn532i2c);

}

  	
bool NFCAuth::setup() {
  Serial.begin(115200);
  Serial.println("Hello!");

  nfc->begin();

  uint32_t versiondata = nfc->getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    return false; // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc->SAMConfig();
  
  Serial.println("Waiting for an ISO14443A Card ...");
  return true;
}


uint8_t* NFCAuth::scan() {
  uint8_t success;
  uint8_t uidLength;                        // Length of the UID 7 bytes for mifare ultralight
    
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, lastUID, &uidLength);
  
  if (success && uidLength == 7) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Value: ");
    nfc->PrintHex(lastUID, uidLength);
    Serial.println("");
    

    Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");
	  return lastUID;

      /*// Try to read the first general-purpose user page (#4)
      Serial.println("Reading page 4");
      uint8_t data[32];
      success = nfc->mifareultralight_ReadPage (4, data);
      if (success)
      {
        // Data seems to have been read ... spit it out
        nfc->PrintHexChar(data, 4);
        Serial.println("");
      }
      else
      {
        Serial.println("Ooops ... unable to read the requested page!?");
      }*/
     
  }
  return nullptr;
}

