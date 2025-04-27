#include "LCD.h"

LCD::LCD(uint8_t Db7, uint8_t Db6, uint8_t Db5, uint8_t Db4, uint8_t RS, uint8_t E){
    this-> Db7 = Db7;
    this->Db6 = Db6;
    this->Db5 = Db5;
    this->Db4 = Db4;
    this->RS = RS;
    this-> E = E;

    //set pinmodes
    pinMode(this->Db4, OUTPUT);
    pinMode(this->Db5, OUTPUT);
    pinMode(this->Db6, OUTPUT);
    pinMode(this->Db7, OUTPUT);
    pinMode(this->RS, OUTPUT);
    pinMode(this->E, OUTPUT);
    delay(10);
    init();
    
}

bool LCD::init(){
    instruct(FUNCTION_SET);
    //delay(25);
    instruct(DISP_ON);
    //delay(25);
    instruct(ENTRY_MODE_SET);
    //delay(25);
    instruct(CLEAR);
    //delay(25);
    return true;
}



bool LCD::clear() {
    instruct(CLEAR);
    return true;
}


//Requires C_string with null term
bool LCD::print(const char* data) {
    if(data == dataLast) {
        return false;
    }
    dataLast = data;
    for(int i = 0; data[i] != '\0'; i++ ){
        write(data[i]);
        //delay(500);
    }
    return true;
}

bool LCD::write(byte data) {
    digitalWrite(RS, 1);
    delayMicroseconds(5);
    instruct(data);
    digitalWrite(RS, 0);
    delayMicroseconds(5);
    return true;
}
//W: 01010111


//Write in MSB format
bool LCD::instruct(byte data) {
    for(int p = 0; p < 2; p++) {
        digitalWrite(E, 1);
        delay(10);
        digitalWrite(Db7, data >> 7 & 1);
        data = data << 1;
        digitalWrite(Db6, data >> 7 & 1);
        data = data << 1;
        digitalWrite(Db5, data >> 7 & 1);
        data = data << 1;
        digitalWrite(Db4, data >> 7 & 1);
        data = data << 1;
        delayMicroseconds(10);
        digitalWrite(E, 0);
        delay(10);
    }
    return true;
    
}