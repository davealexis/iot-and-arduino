# Passworder

## Password Keyboard Emulator



### passworder.ino

```c++
/*
 * ----------------------------------------------------------------------
 * Login assistant using RFID tags
 * ----------------------------------------------------------------------
 *
 * RC522 Wiring for Pro Micro.  The Pro Micro board is used since it is
 * capable of emulating a USB HID (Human Interface Device) like a keyboard
 * or mouse.
 *
 * ----------------------------------
 *             MFRC522
 *             Reader/PCD   Pro Micro
 * Signal      Pin          Pin
 * ----------------------------------
 * RST/Reset   RST          pin 5 (can be any digital pin)
 * SPI SS      SDA(SS)      pin 2 (can be any digital pin)
 * SPI MOSI    MOSI         pin 16
 * SPI MISO    MISO         pin 14
 * SPI SCK     SCK          pin 15
 * 3.3V        3.3V         power pin from 3.3V regulator
 * GND         GND          GND
 * ----------------------------------------------------------------------
 *
 * accounts.h defines the Account struct (tag ID, name, and password)
 * and an array of Accounts.
 *   
 *   Account accounts[] = {
 *       {
 *            { 0xFF, 0xFF, 0xFF, 0xFF },
 *            "Account Name",
 *            "password"
 *       },
 *       . . .
 *   };
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Keyboard.h>
#include "accounts.h"

// Uncomment this line to enable debug output in the Serial monitor
// #def DEBUG		

const uint8_t RST_PIN = 5;     // Configurable, see pin layout above
const uint8_t SS_PIN  = 2;     // Configurable, see pin layout above
const uint8_t ID_SIZE = 4;

MFRC522 rfid(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key key;

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
    
    SPI.begin();
    rfid.PCD_Init();    // Initialize the MFRC522
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
    if ( !rfid.PICC_ReadCardSerial() )
        return;

    
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
    
    #ifdef DEBUG
    Serial.print(F("Tag type: "));
    Serial.println(rfid.PICC_GetTypeName(piccType));
    #endif
    
    #ifdef DEBUG
    printHex(tagId, 4);
    Serial.println();
	#endif
    
    // Get tag's ID
    for (byte i = 0; i < ID_SIZE; i++) {
        tagId[i] = rfid.uid.uidByte[i];
    }

    int accountIndex = findAccount(tagId);
    
    if (accountIndex != -1) {
        Account account = accounts[accountIndex];

        #ifdef DEBUG
        Serial.println(account.title);
        #else
        Keyboard.println(account.password);
        #endif
    }

    #ifdef DEBUG
    Serial.println(F("Tag ID:"));
    printHex(tagId, tagId.size);
    Serial.println();
    Serial.print(F(" ("));
    printDec(tagId, tagId.size);
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
```



### accounts.h

```c++
// Account struct
// This data structure is used to hold the RFID tag ID, title and password
// for an account.
struct Account {
    byte id[4];
    char *title;
    char *password;
};

// Defines the accounts[] array with your account info.
// Replace the example entries below with your information.
Account accounts[] = {
    {
        { 0xA9, 0x10, 0xF8, 0x4E },
        "Key 1",
        "my password"
    },
    {
        { 0x33, 0xFA, 0x8F, 0x1B},
        "Key 2",
        "my password too"
    }
};

```

