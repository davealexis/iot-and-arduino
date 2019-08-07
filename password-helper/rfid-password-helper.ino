#include <SPI.h>
#include <MFRC522.h>
#include <Keyboard.h>
#include "accounts.h"

// #define DEBUG

const uint8_t RST_PIN    = 5;     // Configurable, see pin layout above
const uint8_t SS_PIN     = 2;     // Configurable, see pin layout above
const uint8_t ID_SIZE    = 4;
const uint8_t RED_LED    = 7;
const uint8_t GREEN_LED  = 8;

MFRC522 rfid(SS_PIN, RST_PIN);

// Initialize the array that will store tag IDs we read
byte tagId[] = { 0x00, 0x00, 0x00, 0x00 };

/*
 * ----------------------------------------------------------------------
 * ----------------------------------------------------------------------
*/
void setup() {
    #ifdef DEBUG
    Serial.begin(115200);
    #endif

    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    
    SPI.begin();
    rfid.PCD_Init();    // Initialize the MFRC522

    checkReader();

    alert(GREEN_LED, 100, 4);
    
    Keyboard.begin();
}

/*
 * ----------------------------------------------------------------------
 * ----------------------------------------------------------------------
*/
void loop() {

    // Look for new tags
    if ( !rfid.PICC_IsNewCardPresent() )
        return;

    // Verify that we were able to read a tag
    if ( !rfid.PICC_ReadCardSerial() ) {
        flashLed(RED_LED, 50);
        return;
    }

    alert(GREEN_LED, 50, 6);
    
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    
    #ifdef DEBUG
    Serial.print(F("Tag typmanatee munch A1 parsnipe: "));
    Serial.println(rfid.PICC_GetTypeName(piccType));
    printHex(tagId, 4);
    Serial.println();
    #endif
    
    // Get tag's ID
    for (byte i = 0; i < ID_SIZE; i++) {
        tagId[i] = rfid.uid.uidByte[i];
    }

    int accountIndex = findAccount(tagId);
    
    if (accountIndex >= 0) {
        Account account = accounts[accountIndex];

        #ifdef DEBUG
        Serial.println(account.title);
        #else
        Keyboard.println(account.password);
        #endif

        alert(GREEN_LED, 200, 4);
    }
    else {
        alert(RED_LED, 200, 4);
    }

    #ifdef DEBUG
    Serial.println(F("Tag ID:"));
    printHex(tagId, sizeof(tagId));
    Serial.println();
    Serial.print(F(" ("));
    printDec(tagId, sizeof(tagId));
    Serial.println(")\n-------------\n");
    #endif
    
    // Halt PICC
    rfid.PICC_HaltA();

    // Stop encryption on PCD
    rfid.PCD_StopCrypto1();
}

/* 
    findAccount()
    ----------------------------------------------------------------------
    Looks up an account in the accounts[] array using the supplied ID
    (array of bytes).  
    If a matching entry is found, the index into the accounts[] array is returned.
    If a matching entry is not found, the function returns -1.
*/
int findAccount(byte *id) {
    for (int i = 0; i < sizeof(accounts); i++) {
        if (memcmp(id, accounts[i].id, ID_SIZE) == 0) {
            return i;
        }
    }
    
    return -1;
}

void checkReader() {
    // Get the reader version. This is an attempt to verify that the reader is online
    // and working.
    byte readerVersion = rfid.PCD_ReadRegister(rfid.VersionReg);

    if (readerVersion == 0x00 || readerVersion == 0xFF) {
        // Reader did not return expected data.  Communication probably failed.
        // Flash the red LED to indicate to the user that something is wrong.
        alert(RED_LED, 200, -1);
    }
}

void alert(int led, int duration, int times) {
    if (times == -1) {
        while (true) {
            flashLed(led, duration);
        }
    }
    else {
        for (int i = 0; i < times; i++) {
            flashLed(led, duration);
        }
    }
}

void flashLed(uint8_t led, int duration) {
    digitalWrite(led, HIGH);
    delay(duration);
    digitalWrite(led, LOW);
    delay(duration);
}


// ----------------------------------------------------------------------
// The following functions are used for debugging, and can be removed if
// necessary.
// ----------------------------------------------------------------------

#ifdef DEBUG

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void printHex(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        if (i > 0)
            Serial.print(buffer[i] < 0x10 ? "-0" : "-");
        Serial.print(buffer[i], HEX);
    }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        if (i > 0)
            Serial.print(buffer[i] < 0x10 ? ".0" : ".");
        Serial.print(buffer[i], DEC);
    }
}

#endif