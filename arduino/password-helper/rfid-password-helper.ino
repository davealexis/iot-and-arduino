#include <SPI.h>
#include <MFRC522.h>
#include <Keyboard.h>
#include <FastLED.h>
#include "accounts.h"

// #define DEBUG

#define NUM_LEDS 1
#define LED_PIN 6
#define COLOR_ORDER GRB
const uint8_t RST_PIN     = 5;     // Configurable, see pin layout above
const uint8_t SS_PIN      = 2;     // Configurable, see pin layout above
const uint8_t ID_SIZE     = 4;

enum Colors {
    Red,
    Green,
    Blue
};

CRGB leds[NUM_LEDS];
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

    FastLED.addLeds<WS2812, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(100);
    
    SPI.begin();
    rfid.PCD_Init();    // Initialize the MFRC522

    checkReader();

    alert(Red, 100, 1);
    alert(Green, 100, 1);
    alert(Blue, 100, 1);
    alert(Red, 100, 1);
    alert(Green, 100, 1);
    alert(Blue, 100, 1);
    alert(Red, 100, 1);
    alert(Green, 100, 1);
    alert(Blue, 100, 1);
    
    Keyboard.begin();
}

/*
 * ----------------------------------------------------------------------
 * Main Loop
*/
void loop() {

    // Look for new tags
    if ( !rfid.PICC_IsNewCardPresent() )
        return;

    // Verify that we were able to read a tag
    if ( !rfid.PICC_ReadCardSerial() ) {
        flashLed(Red, 50);
        return;
    }

    alert(Green, 50, 6);
    
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

        alert(Green, 200, 4);
    }
    else {
        alert(Red, 200, 4);
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

/** 
 *  findAccount()
 *   ----------------------------------------------------------------------
 *   Looks up an account in the accounts[] array using the supplied ID
 *   (array of bytes).  
 *   If a matching entry is found, the index into the accounts[] array is returned.
 *   If a matching entry is not found, the function returns -1.
 */
int findAccount(byte *id) {
    for (int i = 0; i < sizeof(accounts); i++) {
        if (memcmp(id, accounts[i].id, ID_SIZE) == 0) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Ensure that we can interact with the RFID reader.
 */
void checkReader() {
    // Get the reader version. This is an attempt to verify that the reader is online
    // and working.
    byte readerVersion = rfid.PCD_ReadRegister(rfid.VersionReg);

    if (readerVersion == 0x00 || readerVersion == 0xFF) {
        // Reader did not return expected data.  Communication probably failed.
        // Flash the red LED to indicate to the user that something is wrong.
        alert(Red, 200, -1);
    } else {
        alert(Blue, 500, 4);
    }
}

/**
 * This function wraps the flashLed() function, and repeats the flash a specified number
 * of times.  If -1 is specified as the number of times, alert() will repeat the flashing
 * forever.
 */
void alert(Colors color, int duration, int times) {
    if (times == -1) {
        while (true) {
            flashLed(color, duration);
        }
    }
    else {
        for (int i = 0; i < times; i++) {
            flashLed(color, duration);
        }
    }
}

/**
 * Flash a given color of the RGB Neopixel for the specified duration.
 * This turns the LED on with the specified color, then turns it off.
 */
void flashLed(Colors color, int duration) {
    leds[0] = CRGB(
        color == Red ? 255 : 0, 
        color == Green ? 255 : 0, 
        color == Blue ? 255 : 0);
    FastLED.show();
    delay(duration);
    leds[0] = CRGB(0, 0, 0);
    FastLED.show();
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