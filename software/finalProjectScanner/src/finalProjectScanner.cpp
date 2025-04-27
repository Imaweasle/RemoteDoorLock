/* 
 * Project myProject
 * Author: Your Name
 * Date: 
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"
#include "NFC.h"
#include "LCDDEFS.h"
#include "LiquidCrystal_I2C.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
//SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool correctUID;
uint8_t *uid = nullptr;

bool lastManualLock = false;
bool manualLock = true;

const uint8_t goodUID[7] = {0x04, 0x4C, 0x14, 0x29, 0xB7, 0x2A, 0x81}; 


NFCAuth nfc;
// setup() runs once, when the device is first turned on
void setup() {
  // Put initialization like pinMode and begin functions here
  delay(1000);
  nfc.setup();

  //Setup LCD
  lcd.init();
  lcd.backlight();
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

  
  if(!manualLock) {
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
        while(uid != nullptr) {
          uid = nfc.scan();
          //Particle.process();
          delay(200);
        }
      }
    }
    lastManualLock = manualLock;
  } else {
    if(lastManualLock != manualLock) {
      lcd.clear();
      lcd.print(LOCKED);
    }
    //Insert Comms Code
    lastManualLock = manualLock;
  }
    
  

  

}


