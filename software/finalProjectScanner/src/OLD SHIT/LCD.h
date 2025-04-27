#pragma once

/*
*   This is a driver for runing the SPLC780C LCD controller with a 74HC595 shift register
*
*
*/

//Byte Defs for instructions
#include "LCDDEFS.h"

#include "Particle.h"


class LCD {
    private:
        uint8_t Db7;
        uint8_t RS;
        uint8_t Db6;
        uint8_t Db5;
        uint8_t Db4;
        uint8_t E;
        short step = 0;
        short lastStep = 0;
        const char* dataLast = nullptr;

        bool lastState = 0;
        
        //pass instruction
        bool instruct(byte data);
        //pass data to ram
        bool write(byte data);
    
    public:
        LCD(uint8_t Db7, uint8_t Db6, uint8_t Db5, uint8_t Db4, uint8_t RS, uint8_t E);

        //Setup screen in 8 bit two line mode with cursor off
        bool init();
        
        //this method was made to test as I was having issues, wound up having a faulty 74HC595
        void testLoop(uint8_t stepPort);

        //Clear screen
        bool clear();

        //Print cstring out to display
        bool print(const char* data);


};
