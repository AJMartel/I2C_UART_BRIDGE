/*
 * This is code was made with the intention to convert I2C to UART Or UART to I2C
 * By sending in the first String of the COMMAND[] String, settings can be changed
 * using the follring syntax:   COMMAND[0]COMMAND[x]val,COMMAND[n]
 * It is adviced to clear the EEPROM once the code has been uploade
 * this is done by sending COMMAND[0]COMMAND[7]==> !#RST
 * COMMANDS[] = {"!#", "BAUD", "ADDR", "I2CC", "BUFL", "MSTR", "SLVD", "RST", "TIMO", "SLND"}
 *                setting  // BAUD(val) // ADDR(1-128)//I2CC(100000, 400000)//BUFL(1 - MAXI2CBUFFERLENGTH)//MSTR//SLVD//RST//TIMO(val)//SLND(0-1)
 *                         //uart BAUD // I2C address  // I2C clock speed /// I2C BUFFER SIZE  // I2C MASTER MODE// I2C SLAVE MODE // RESET SETTINGS // TIMOUT // SILEND (no echo)
 *  
 */

#include <EEPROM.h>
//please wipe your eeprom first
#include <Wire.h>
unsigned long UARTBAUD        = 250000;    // aurt speed
uint8_t       I2CADDRESS      = 8;         // slave address
unsigned long I2CSPEED        = 400000L;    // i2c data speed
unsigned long BUFFERLENGTH    = 128;        // length of I2C buffer
const unsigned long MAXI2CBUFFERLENGTH = 256; // max length of I2C buffer
unsigned long TIMEOUT  = 1000;// master request timeout in millis
uint8_t MAXI2C_DATA = 32;// max number of bytes that the hardware can transfer
String MASTERREQUEST = "RQST";
String COMMANDS[] = {"!#", "BAUD", "ADDR", "I2CC", "BUFL", "MSTR", "SLVD", "RST", "TIMO", "SLND"}; // first is to trigger configuration mode, commands for setting uart baud, i2c address, i2c clock speed and buffer length
char delim = ',';
bool FirstRunCheck   = false ; // check if sketch ran  afirst time
bool MASTERMODE      = true;          // if 1 device is master if 0 slave
bool silent          = false;
byte boolsettings;//
String BAUDS = "9600, 19200, 38400, 57600, 115200, 250000, 460800"; // list of bauds alloud
String I2CSPEEDS = "400000, 100000";// alloud I2C speeds in khz

uint16_t eepromAddress[]  = {0, 4, 8, 12, 16, 20};// data address on eeprom 27 is first time rum check


uint16_t I2CREQBUFINTERVAL;// I2C REQUESTS ARE SEND IN PACKAGES OF SIZE MAXI2C_DATA, THIS WILL HOLD THE INTERVAL

char UARTBUFFER[MAXI2CBUFFERLENGTH];

void settings(bool set) { // true to read settings, false to store settings

    boolsettings = EEPROM.read(eepromAddress[0]);

    FirstRunCheck = bitRead(boolsettings, 0);

    if(FirstRunCheck && set == false) { //READ SETTINGS IF SKETCH ALREADY RAN A FIRST TIME

        EEPROM.get(eepromAddress[1], UARTBAUD);
        EEPROM.get(eepromAddress[2], I2CADDRESS);
        EEPROM.get(eepromAddress[3], I2CSPEED);
        EEPROM.get(eepromAddress[4], BUFFERLENGTH);
        EEPROM.get(eepromAddress[5], TIMEOUT);
        MASTERMODE    = bitRead(boolsettings, 1);
        silent        = bitRead(boolsettings, 2);

    } else if( !FirstRunCheck || set) { // save settings in eeprom
        bitWrite(boolsettings, 0, 1);
        bitWrite(boolsettings, 1, MASTERMODE);
        bitWrite(boolsettings, 2, silent);

        EEPROM.write(eepromAddress[0], boolsettings);
        EEPROM.put(eepromAddress[1], UARTBAUD);
        EEPROM.put(eepromAddress[2], I2CADDRESS);
        EEPROM.put(eepromAddress[3], I2CSPEED);
        EEPROM.put(eepromAddress[4], BUFFERLENGTH);
        EEPROM.put(eepromAddress[5], TIMEOUT);
    }

}

void wipeEEPROM() { // erase all data on eeprom the at least once after startup;
    uint32_t p = 0;
    EEPROM.put(eepromAddress[0], p);
    EEPROM.put(eepromAddress[1], p);
    EEPROM.put(eepromAddress[2], p);
    EEPROM.put(eepromAddress[3], p);
    EEPROM.put(eepromAddress[4], p);
    EEPROM.put(eepromAddress[5], p);
}

uint8_t LED = 13;
long time = 500;
void setup() {

    settings(false);
    Serial.begin(UARTBAUD);      // initialize serial port
    Wire.setClock(I2CSPEED);     // set I2C clock speed
    pinMode(LED, OUTPUT);

    Wire.onReceive(I2CreceiveEvent); // register receive event
    if(MASTERMODE) {
        Wire.begin();
        if(!silent) {
            Serial.println("I2C MASTER MODE SPEED(HZ): "+String(I2CSPEED));
        }
    } else {
        Wire.onRequest(I2CWRITE); // register request event
        Wire.begin(I2CADDRESS);         // set  I2C as slave on address if 0 master else slave
        if(!silent) {
            Serial.println("I2C SLAVE MODE ADDRESS: "+String(I2CADDRESS)+" SPEED: "+String(I2CSPEED)+" HZ");
        }
    }

    if(MASTERMODE) {
        time = 250;
    }
}


void loop() {
    /// wait for events to happen
    serialEvenT();
    blink();
}

void blink() {
    static unsigned long timer;
    static bool state;


    if(millis() - timer > time) {
        timer = millis();
        digitalWrite(LED, state);
        if(state) {
            state = false;;
        } else {
            state = true;
        }
    }
}


void masterRequest(uint8_t addr, uint16_t length) {
    uint16_t req;

    while(length > 0 ) {

        req = length % MAXI2C_DATA;
        if(req == 0) {
            req = MAXI2C_DATA;
        }

        Wire.requestFrom(addr, req);// request bytes from slave device

        while (Wire.available()) { // slave may send less than requested
            char c = Wire.read(); // receive a byte as character
            Serial.print(c);         // print the character
            length--;// substract the byte that was send
        }
    }


}


void I2CTransmit() {

    int16_t p = BUFFERLENGTH / MAXI2C_DATA;
    uint16_t count = 0;
    while(count < p) {

        Wire.beginTransmission(I2CADDRESS); // transmit to device #8
        uint16_t from = (count++ * MAXI2C_DATA);
        uint16_t to = (count + 1) * MAXI2C_DATA;//

        for(uint16_t i = from; i < to; i++) {
            Wire.write(UARTBUFFER[i]);  // WRITE DATA
            if(i == BUFFERLENGTH) {
                break;
            }
        }
        Wire.endTransmission();    // stop transmitting
    }

}

void serialEvenT() { // on serial event, data will be stored in the UARTBUFFER
    if(Serial.available() > 0) {

        String data = Serial.readString();// read the incoming string
        data.trim();
        uint16_t data_size = data.length();

        for(uint16_t i = 0; i < BUFFERLENGTH; i++) { // convert the String into a char array
            if(i <= data_size) {
                UARTBUFFER[i] = data.charAt(i);
            } else {
                UARTBUFFER[i] = ' ';
            }
        }
        parseCommand(data);
    }
}




void parseCommand(String data) {
    uint16_t data_size = data.length();
    data.trim();

    String p;
    for(uint16_t i = 0; i < COMMANDS[0].length(); i++) { // convert the String into a char array
        p += data.charAt(i);
    }


    if(p == COMMANDS[0]) {
        bool wipe = false;
        int16_t index;
        data = data.substring(data.indexOf(COMMANDS[0]) + COMMANDS[0].length());// remove the command string

        data.trim();
        for(uint8_t i = 1; i < 10; i++) {

            index = data.indexOf(COMMANDS[i]);

            if (index >= 0) {

                unsigned long val = (data.substring(index + COMMANDS[i].length())).toInt();

                switch(i) {
                case 1:// configure aurt baud

                    if( BAUDS.indexOf(String(val)) >= 0 && val > 10) {
                        UARTBAUD = val;
                        if(!silent) {
                            Serial.println("UART BAUD CHANGED TO: "+String(UARTBAUD)+" BPS");
                        }
                    } else {
                        if(!silent) {
                            Serial.println("ERROR BAUD NOT PERMITTED: "+BAUDS);
                        }
                    }

                    break;

                case 2: // set I2C addres that will be set as slave and will write to as master
                    if(val > 0 && val < 128) {
                        I2CADDRESS = val;
                        if(!silent) {
                            Serial.println("I2C ADDRESS CHANGED TO: "+String(I2CADDRESS));
                        }
                    } else {
                        if(!silent) {
                            Serial.println("ERROR I2C ADDRESS NOT PERMITTED: 1 - 128");
                        }
                    }

                    break;

                case 3:/// set i2c clock speed
                    if( I2CSPEEDS.indexOf(String(val)) >= 0 && val > 10) {
                        I2CSPEED = val;
                        if(!silent) {
                            Serial.println("I2C CLOCK SPEED SET TO: "+String(I2CSPEED));
                        }
                    } else {
                        if(!silent) {
                            Serial.println("ERROR I2C SPEED NOT PERMITTED: "+I2CSPEEDS);
                        }
                    }
                    break;

                case 4:// set i2c buffer size
                    if(val < MAXI2CBUFFERLENGTH && val > 0) {
                        BUFFERLENGTH = val;
                        if(!silent) {
                            Serial.println("BUFFER LENGTH SET: "+String(BUFFERLENGTH));
                        }
                    } else {
                        if(!silent) {
                            Serial.println("ERROR BUFFERLENGTH NOT PERMITED: 1 - "+String(MAXI2CBUFFERLENGTH)+" BYTES");
                        }
                    }
                    break;

                case 5:// set master mode after reset
                    MASTERMODE = true;
                    if(!silent) {
                        Serial.println("SETTING TO ASTER MODE");
                    }
                    break;

                case 6: // set slave mode after reset
                    MASTERMODE = false;
                    if(!silent) {
                        Serial.println("SETTING TO SLAVE MODE ON ADDRESS: "+String(I2CADDRESS));
                    }
                    break;

                case 7: // reset settings
                    if(!silent) {
                        Serial.println("RESTORING SETTINGS TO DEFAULT PLEASE WAIT");
                    }
                    wipeEEPROM();
                    wipe = true;
                    break;

                case 8: // reset settings
                    TIMEOUT = val;
                    if(!silent) {
                        Serial.println("MASTER REQUEST TIMEOUT SET TO: "+String(TIMEOUT)+" MILLISECONDS");
                    }
                    break;
                case 9: // silent mode setting
                    if(val == 0 || val == 1) {
                        silent = val;
                        bitWrite(boolsettings, 2, val);
                        if(!silent) {
                            Serial.println("SILENT MODE SET TO: "+String(val));
                        }
                    } else {
                        if(!silent) {
                            Serial.println("ERROR SILENT MODE: 0, 1");
                        }
                    }

                    break;

                }
            }
        }

        if(wipe == false) {
            settings(true);   //save settings to eeprom
        }

        if(!silent) {
            Serial.println(" SETTINGS WILL BE APPLIED AFTER REBOOT");
        }
    } else if(MASTERMODE) { // start transmission over I2C

        String x;
        for(uint16_t i = 0; i < MASTERREQUEST.length(); i++) { // convert the String into a char array
            x += data.charAt(i);
        }

        if(x == MASTERREQUEST) {
            uint8_t addr;
            uint16_t length;

            data = data.substring(data.indexOf(MASTERREQUEST) + MASTERREQUEST.length());// parse slave address
            addr = data.toInt();

            if(data.indexOf(delim) > 0 ) {
                data = data.substring(data.indexOf(delim) + 1);// parse bytes count
                length = data.toInt();
            } else {
                length = 0;
            }


            if(addr != 0 && length == 0) {
                length = addr;
                addr = I2CADDRESS;
            } else {
                if(addr == 0) {
                    addr = I2CADDRESS;
                } else {
                    I2CADDRESS = addr;
                }
            }

            if(length == 0 || length > MAXI2CBUFFERLENGTH) {
                length = BUFFERLENGTH;
            } else {
                BUFFERLENGTH = length;
            }

            if(!silent) {
                Serial.println("I2C MASTER REQUEST: "+String(addr)+", "+String(length));
            }

            masterRequest(addr, length);
        } else {
            I2CTransmit();
        }
    } else {
        I2CREQBUFINTERVAL = 0;// reset I2C request val;

    }
}


void I2CreceiveEvent() {// on I2C event, chars will be written directly to the UART
    
    String data;
    while (Wire.available() > 0) {    // read all char on I2C buffer
        char c = Wire.read();           // receive byte as a character
        data += c;
        Serial.print(c);
    }
    parseCommand(data);// check commands

}


void I2CWRITE() { //

    uint16_t from = (I2CREQBUFINTERVAL * MAXI2C_DATA);
    uint16_t to = (I2CREQBUFINTERVAL + 1) * MAXI2C_DATA;//
    if(to > BUFFERLENGTH) {
        to = BUFFERLENGTH;
    }

    uint16_t i = from;

    while( i < to) {
        Wire.write(UARTBUFFER[i++]);// WRITE DATA
    }
    I2CREQBUFINTERVAL++;
    if(i >= BUFFERLENGTH) {
        I2CREQBUFINTERVAL = 0;
    }
}
