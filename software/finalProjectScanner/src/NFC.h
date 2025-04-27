#include "Particle.h"
#include <Wire.h>
#include <inttypes.h>
#include "PN532_I2C.h"
#include "PN532.h"

//heavily influenced from examples in library

class NFCAuth {
    private:
        PN532_I2C *pn532i2c;
        PN532 *nfc;
        uint8_t lastUID[7] = {0, 0, 0, 0, 0, 0, 0};

        //updates the key
        bool keyInc();
    public:
        NFCAuth();
        bool setup();
        uint8_t* scan();

        //checks that the card hasn't been tampered with
        bool check();

        bool checkKey();
        



};


