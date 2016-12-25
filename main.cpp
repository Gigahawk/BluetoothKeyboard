#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined(ARDUINO_ARCH_SAMD)
#include <SoftwareSerial.h>
#endif

#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>
#include <Adafruit_BluefruitLE_UART.h>

#include "BluefruitConfig.h"

#define SS 5

#define LED 13

#define FACTORYRESET_ENABLE 0
#define MINIMUM_FIRMWARE_VERSION "0.6.6"

#define SHIFT(a) addtoBuffer(a); addModifier(0x02);
#define CTRL(a) addtoBuffer(a); addModifier(0x01);
#define ALT(a) addtoBuffer(a); addModifier(0x04);

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

int keybuffer[8] = {0};
int bufferindex = 2;

char key, tempkey;

void setup() {
    pinMode(SS, INPUT);
    pinMode(LED, OUTPUT);
    Serial.begin(9600);
    Serial1.begin(9600);

    while (!Serial);

    Serial.println("Starting Bluetooth keyboard");

    ble.begin(VERBOSE_MODE);
    ble.echo(false);

    ble.sendCommandCheckOK("AT+GAPDEVNAME=Bluefruit Keyboard");
    ble.sendCommandCheckOK("AT+BleHIDEn=On");
    ble.reset();



}

void loop() {
    readKey();
    delay(5);
}

int isInBuffer(int hexval) {
    for (int i = 0; i < 8; i++) {
        if (keybuffer[i] == hexval) {
            return i;
        }
    }
    return -1;
}

void readKey() {
    if (digitalRead(SS) == HIGH) {
        digitalWrite(LED, HIGH);
        if (Serial1.available()) {
            tempkey = Serial1.read();
            Serial.print("Read '");
            Serial.print(tempkey);
            Serial.print("', DEC ");
            Serial.println(tempkey,DEC);
            if (key != tempkey) {
                Serial.println("Overwriting key...");
                key = tempkey;
                processKey();
                sendBuffer();
            }
        }
    } else {
        digitalWrite(LED, LOW);
        key = 0;
        clearBuffer();
        sendBuffer();
    }
}

void readWindows() {
    if (Serial1.available()) {
        key = Serial1.read();
        if (key == 27) {
            processEscKey();
        }
    }
}

void processKey() {
    if (key == 27) {
        Serial.println("Got an escape key");
        delay(5);
        if (Serial1.available() > 0) {
            key = Serial1.read();
            Serial.print("Escape key is ");
            Serial.println(key, HEX);
        }
        processEscKey();
    } else if (key <= 26 && key != 8 && key != 9 && key != 10 && key != 13) {
        Serial.println("Got a control key");
        processCtrlKey();
    } else {
        Serial.print("Got a ");
        Serial.print("[");
        Serial.print(key);
        Serial.println("]");
        processCharKey();
    }
}

void processCtrlKey() {
    CTRL(key+3);
}

void processCharKey() {
    if (key >= 65 && key <= 90) { //Upper case characters
        SHIFT(key-61);
    } else if (key >= 97 && key <= 122) { //Lower case characters
        writeBuffer(0,0); //Clear a held down Shift
        addtoBuffer(key - 93);
    } else if (key >= 49 && key <= 57) { //Numbers 1-9
        addtoBuffer(key -19);
    } else {
        switch (key) {
        case 8: //[BACKSPACE]
            addtoBuffer(0x2A);
            break;
        case 9: //[TAB]
            addtoBuffer(0x2B);
            break;
        case 10: //[ENTER]
            addtoBuffer(0x28);
            break;
        case 32: //[SPACE]
            addtoBuffer(0x2C);
            break;
        case 33: //!
            SHIFT(0x1E);
            break;
        case 34: //"
            SHIFT(0x34);
            break;
        case 35: //#
            SHIFT(0x20);
            break;
        case 36: //$
            SHIFT(0x21);
            break;
        case 37: //%
            SHIFT(0x22);
            break;
        case 38: //&
            SHIFT(0x24);
            break;
        case 39: //'
            addtoBuffer(0x34);
            break;
        case 40: //(
            SHIFT(0x26);
            break;
        case 41: //)
            SHIFT(0x27);
            break;
        case 42: //*
            SHIFT(0x25);
            break;
        case 43: //+
            SHIFT(0x2E);
            break;
        case 44: //,
            addtoBuffer(0x36);
            break;
        case 45: //-
            addtoBuffer(0x2D);
            break;
        case 46: //.
            addtoBuffer(0x37);
            break;
        case 47: //[SLASH]
            addtoBuffer(0x38);
            break;
        case 48: //0
            addtoBuffer(0x27);
            break;
        case 58: //:
            SHIFT(0x33);
            break;
        case 59: //;
            addtoBuffer(0x33);
            break;
        case 60: //<
            SHIFT(0x36)
            break;
        case 61: //=
            addtoBuffer(0x2E);
            break;
        case 62: //>
            SHIFT(0x37);
            break;
        case 63: //?
            SHIFT(0x38);
            break;
        case 64: //@
            SHIFT(0x1F);
            break;
        case 91: //[
            addtoBuffer(0x2F);
            break;
        case 92: //[BACKSLASH]
            addtoBuffer(0x31);
            break;
        case 93: //]
            addtoBuffer(0x30);
            break;
        case 94: //^
            SHIFT(0x23);
            break;
        case 95: //_
            SHIFT(0x2D);
            break;
        case 96: //` NOTE: No HID code, using ALT-Code as a workaround
            clearBuffer();
            addtoBuffer(0x53);// Num Lock Enable
            sendBuffer();
            clearBuffer();
            ALT(0x61); //Alt + Numpad 9
            addtoBuffer(0x5E); //Numpad 6
            sendBuffer();
            clearBuffer();
            addtoBuffer(0x53);// Num Lock Disable
            sendBuffer();
            clearBuffer();
            break;
        case 123: //{
            SHIFT(0x2F);
            break;
        case 124: //|
            SHIFT(0x31);
            break;
        case 125: //}
            SHIFT(0x30);
            break;
        case 126: //~ NOTE: HID code doesn't work properly, using ALT-Code as a workaround
            clearBuffer();
            addtoBuffer(0x53);// Num Lock Enable
            sendBuffer();
            clearBuffer();
            ALT(0x59); //Alt + Numpad 1
            addtoBuffer(0x5A); //Numpad 2
            addtoBuffer(0x5E); //Numpad 6
            sendBuffer();
            clearBuffer();
            addtoBuffer(0x53);// Num Lock Disable
            sendBuffer();
            clearBuffer();
            break;
        }
    }
}

void addtoBuffer(int hexval) {
    int duplicate = isInBuffer(hexval);
    if (duplicate != -1) {
        Serial.println("Duplicate key found!");
        writeBuffer(duplicate, 0);
        sendBuffer();
    }
    writeBuffer(bufferindex, hexval);


    bufferindex++;
    if (bufferindex > 7) {
        bufferindex = 2;
    }
}

void addModifier(int hexval) {
    Serial.print("Adding modifier ");
    Serial.println(hexval, HEX);
    keybuffer[0] |= hexval;
}

void writeBuffer(int index, int hexval) {
    Serial.print("Writing ");
    Serial.print(hexval, HEX);
    Serial.print(" to index ");
    Serial.println(index);
    keybuffer[index] = hexval;
}

void clearBuffer() {
    //Serial.println("Clearing Buffer");
    for (int i = 0; i < 8; i++) {
        writeBuffer(i, 0);
    }
    bufferindex = 2;
}

void processEscKey() {

    if (key >= 0x68 && key <= 0x71) {
        Serial.print("[Alt-F");
        Serial.print(key-103);
        Serial.println("]");
        ALT(key - 0x2E);
    }

    switch (key) {
    case 27: //Escape
        Serial.println("[Esc]");
        addtoBuffer(0x29);
        break;
    case 0x49: //Page Up
        Serial.println("[PgUp]");
        addtoBuffer(0x4B);
        break;
    case 0x51: //Page Down
        Serial.println("[PgDn]");
        addtoBuffer(0x4E);
        break;
    case 0x47: //Home
        Serial.println("[Home]");
        addtoBuffer(0x4A);
        break;
    case 0x4F: //End
        Serial.println("[End]");
        addtoBuffer(0x4D);
        break;
    case 0x52: //Insert
        Serial.println("[Ins]");
        addtoBuffer(0x49);
        break;
    case 0x53: //Delete
        Serial.println("[Del]");
        addtoBuffer(0x4C);
        break;
    case 0x3B: //F1
        Serial.println("[F1]");
        addtoBuffer(0x3A);
        break;
    case 0x3C: //F2
        Serial.println("[F2]");
        addtoBuffer(0x3B);
        break;
    case 0x3D: //F3
        Serial.println("[F3]");
        addtoBuffer(0x3C);
        break;
    case 0x3E: //F4
        Serial.println("[F4]");
        addtoBuffer(0x3D);
        break;
    case 0x3F: //F5
        Serial.println("[F5]");
        addtoBuffer(0x3E);
        break;
    case 0x40: //F6
        Serial.println("[F6]");
        addtoBuffer(0x3F);
        break;
    case 0x41: //F7
        Serial.println("[F7]");
        addtoBuffer(0x40);
        break;
    case 0x42: //F8
        Serial.println("[F8]");
        addtoBuffer(0x41);
        break;
    case 0x43: //F9
        Serial.println("[F9]");
        addtoBuffer(0x42);
        break;
    case 0x44: //F10
        Serial.println("[F10]");
        addtoBuffer(0x43);
        break;
    case 0x57: //F11
        Serial.println("[F11]");
        addtoBuffer(0x44);
        break;
    case 0x58: //F12
        Serial.println("[F12]");
        addtoBuffer(0x45);
        break;
    case 0x48: //Up
        Serial.println("[Up]");
        addtoBuffer(0x52);
        break;
    case 0x50:
        Serial.println("[Down]");
        addtoBuffer(0x51);
        break;
    case 0x4B:
        Serial.println("[Left]");
        addtoBuffer(0x50);
        break;
    case 0x4D:
        Serial.println("[Right]");
        addtoBuffer(0x4F);
        break;
    case 0x54:
        Serial.println("[Print]");
        addtoBuffer(0x46);
        break;
    case 0x5B: //Left Windows
        Serial.println("[Windows]");
        addModifier(0x08);
        WindowsOverride();
        break;

        //Seriously why would you order the keys like this it makes everything stupidly complicated
    case 0x1E: //Alt-A
        Serial.println("[Alt-A]");
        ALT(0x04);
        break;
    case 0x30: //Alt-B
        Serial.println("[Alt-B]");
        ALT(0x05);
        break;
    case 0x2E:
        Serial.println("[Alt-C]");
        ALT(0x06);
        break;
    case 0x20:
        Serial.println("[Alt-D]");
        ALT(0x07);
        break;
    case 0x12:
        Serial.println("[Alt-E]");
        ALT(0x08);
        break;
    case 0x21:
        Serial.println("[Alt-F]");
        ALT(0x09);
        break;
    case 0x22:
        Serial.println("[Alt-G]");
        ALT(0x0A);
        break;
    case 0x23:
        Serial.println("[Alt-H]");
        ALT(0x0B);
        break;
    case 0x17:
        Serial.println("[Alt-I]");
        ALT(0x0C);
        break;
    case 0x24:
        Serial.println("[Alt-J]");
        ALT(0x0D);
        break;
    case 0x25:
        Serial.println("[Alt-K]");
        ALT(0x0E);
        break;
    case 0x26:
        Serial.println("[Alt-L]");
        ALT(0x0F);
        break;
    case 0x32:
        Serial.println("[Alt-M]");
        ALT(0x10);
        break;
    case 0x31:
        Serial.println("[Alt-N]");
        ALT(0x11);
        break;
    case 0x18:
        Serial.println("[Alt-O]");
        ALT(0x12);
        break;
    case 0x19:
        Serial.println("[Alt-P]");
        ALT(0x13);
        break;
    case 0x10: //Substitute for Alt-Tab (Not natively supported by adapter)
        // TODO: Find fix for Alt being released when Q is released
        Serial.println("[Alt-Q] (Sending Alt-Tab Instead)");
        ALT(0x2B);
        break;
        //TODO: Finish writing the rest of this
    default:
        Serial.println("[?]");
        break;
    }

}

void WindowsOverride() {
    while (key == 0x5B) {
        readWindows();
    }
}

void sendBuffer() {
    ble.print("AT+BleKeyboardCode=");
    for (int i = 0; i < 7; i++) {
        if (keybuffer[i] < 16)
            ble.print("0");
        ble.print(keybuffer[i],HEX);
        ble.print("-");
    }
    if (keybuffer[7] < 16)
        ble.print("0");
    ble.println(keybuffer[7],HEX);


}
