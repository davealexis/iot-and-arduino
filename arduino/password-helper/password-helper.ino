/*
 * Password Helper - Mark IV
 *
 * This version of the password helper uses the bluetooth capabilities of the ESP32
 * to emulate a bluetooth keyboard. (The previous versions were built with cheap
 * Arduino Pro Micro boards that kept having the USB connectors snap off.)
 * Specifically, an Adafruit Huzzah32 was used for this build because of a couple
 * unique features:
 * - Built-in JST-PH battery connector
 * - Built in lipo battery charger, with automatic switch-over from battery to USB power
 *   when plugged in.
 * - Battery level monitor on pin A13.
 *
 * TODO:
 *    [x] Have a battery level tag.  When scanned, it types out the current battery level.
 *    [ ] Have a tag reader tag. When scanned, it puts the reader into a mode where the
 *        information for the next tag that is read is typed out (tag ID, etc)
 *
 * https://github.com/T-vK/esp32-ble-keyboard
 */

#include <SPI.h>
#include <MFRC522.h>
#include <BleKeyboard.h>
#include <FastLED.h>
#include "driver/rtc_io.h"
#include "accounts.h"

//#define DEBUG

#define NUM_LEDS 1
#define LED_PIN 14
#define RST_PIN  22
#define SS_PIN 23
#define READER_POWER_PIN 32

#define COLOR_ORDER GRB
#define DEVICE_NAME "Alexis Custom Keyboard"
#define MANUFACTURER "David Alexis"
//#define TOUCH_THRESHOLD 40

const gpio_num_t BUTTON_PIN = GPIO_NUM_15;
const uint8_t ID_SIZE = 4;

unsigned long startMillis;
unsigned long currentMillis;
int buttonState;
int lastButtonState;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
bool goodRead;

enum Colors
{
    Red,
    Green,
    Blue,
    Yellow
};

CRGB leds[NUM_LEDS];
MFRC522 rfid(SS_PIN, RST_PIN);
BleKeyboard keyboard(DEVICE_NAME, MANUFACTURER, 100);

// Initialize the array that will store tag IDs we read
byte tagId[] = { 0x00, 0x00, 0x00, 0x00 };


/**
 * ----------------------------------------------------------------------
 */
void setup()
{
    delay(10);
    // ++bootCount;

    #ifdef DEBUG
    Serial.begin(115200);
    #endif

    FastLED.addLeds<WS2812, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(30);

    pinMode(BUTTON_PIN, INPUT);
    pinMode(READER_POWER_PIN, OUTPUT);
    digitalWrite(READER_POWER_PIN, HIGH);

    // Initialize and verify the MFRC522 RFID reader
    SPI.begin();
    rfid.PCD_Init();
    checkReader();

    // Wait for a host device to connect to our bluetooth "keyboard".
    // A red blip will be displayed every half a second until connected.
    // We don't want to burn through battery doing this, so we'll go to sleep
    // after 30 seconds of waiting for a connection.
    keyboard.begin();

    int keyboardTry = 0;

    while (!keyboard.isConnected())
    {
        flashLed(Blue, 50);
        delay(500);

        keyboardTry++;

        if (keyboardTry == 30)
          sleep();
    }

    //readBatteryLevel(true);
    startMillis = millis();
}

/**
 * ----------------------------------------------------------------------
 * Main Loop
 */
void loop()
{
    int buttonReading = digitalRead(BUTTON_PIN);

    if (buttonReading != lastButtonState)
    {
      // reset the debouncing timer
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay)
    {
        // whatever the reading is at, it's been there for longer than the debounce
        // delay, so take it as the actual current state:

        // if the button state has changed:
        if (buttonReading != buttonState)
        {
            buttonState = buttonReading;

          // only toggle the LED if the new button state is HIGH
          if (buttonState == HIGH) {
              buttonState = LOW;
              sleep();
          }
        }
    }

    lastButtonState = buttonReading;
    goodRead = false;

    ledOn(Green);

    // Check if an RFID tag has been scanned.
    // We'll go to sleep after 30 seconds of no tags being detected.
    if (!rfid.PICC_IsNewCardPresent())
    {
        //checkSleepyTime();
        delay(100);
        return;
    }

    // Someone tapped a tag. Let's restart the sleep countdown.
    startMillis = millis();
    ledOn(Green);

    // Verify that we were able to read a tag. If it fails, let the user know.
    if ( !rfid.PICC_ReadCardSerial() )
    {
        alert(Red, 50, 4);
        return;
    }

    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

    #ifdef DEBUG
    Serial.print(F("Tag type: "));
    Serial.println(rfid.PICC_GetTypeName(piccType));
    printHex(tagId, 4);
    Serial.println();
    #endif

    // Get tag's ID
    for (byte i = 0; i < ID_SIZE; i++)
    {
        tagId[i] = rfid.uid.uidByte[i];
    }

    // Now let's look for the tag in our tag collection/password map.
    int accountIndex = findAccount(tagId);

    if (accountIndex >= 0)
    {
        goodRead = true;
        Account account = accounts[accountIndex];

        #ifdef DEBUG
        Serial.println(account.title);
        #endif

        // Notify the user that we got a good tag and found the account, then
        // "type" the password out through the keyboard interface.
        typeText(account.password);
    }
    else
    {
        // We didn't find the tag in our list.  Let the user know.
        alert(Red, 200, 4);
        goodRead = false;
        return;
    }

    #ifdef DEBUG
    Serial.println(F("Tag ID:"));
    printHex(tagId, sizeof(tagId));
    Serial.println();
    Serial.print(F(" ("));
    printDec(tagId, sizeof(tagId));
    Serial.println(")\n-------------\n");
    #endif
}

/**
 * Looks up an account in the accounts[] array using the supplied ID
 * (array of bytes).
 * If a matching entry is found, the index into the accounts[] array is returned.
 * If a matching entry is not found, the function returns -1.
 */
int findAccount(byte *id)
{
    for (int i = 0; i < sizeof(accounts); i++)
    {
        if (memcmp(id, accounts[i].id, ID_SIZE) == 0)
        {
            return i;
        }
    }

    return -1;
}

/**
 * Ensure that we can interact with the RFID reader.
 */
void checkReader()
{
    while (true)
    {
        // Get the reader version. This is an attempt to verify that the reader is online
        // and working.
        byte readerVersion = rfid.PCD_ReadRegister(rfid.VersionReg);

        if (readerVersion == 0x00 || readerVersion == 0xFF)
        {
            #ifdef DEBUG
            Serial.println("Reader failed");
            Serial.println(readerVersion);
            #endif
            // Reader did not return expected data.  Communication probably failed.
            // Flash the red LED to indicate to the user that something is wrong.
            alert(Red, 200, 5);
            delay(1000);
        }
        else
        {
            break;
        }
    }
}

/**
 * This function wraps the flashLed() function, and repeats the flash a specified number
 * of times.  If -1 is specified as the number of times, alert() will repeat the flashing
 * forever.
 */
void alert(Colors color, int duration, int times)
{
    if (times == -1)
    {
        while (true)
        {
            flashLed(color, duration);
        }
    }
    else
    {
        for (int i = 0; i < times; i++)
        {
            flashLed(color, duration);
        }
    }
}

/**
 * Flash a given color of the RGB Neopixel for the specified duration.
 * This turns the LED on with the specified color, then turns it off.
 */
void flashLed(Colors color, int duration)
{
    ledOn(color);
    delay(duration);
    ledOff();
    delay(duration);
}

/**
 * Turns on the neopixel LED with the specified color.
 */
void ledOn(Colors color)
{
    leds[0] = CRGB(
        color == Red || color == Yellow ? 255 : 0,
        color == Green ? 255 : color == Yellow? 250 : 0,
        color == Blue ? 255 : color == Yellow? 205 : 0);
    FastLED.show();
}

/**
 * Turns off the neopixel LED.
 */
void ledOff()
{
    leds[0] = CRGB(0, 0, 0);
    FastLED.show();
}

/**
 * Types out the supplied text to the bluetooth keyboard interface.
 */
void typeText(char *stringToPrint)
{
    ledOn(Blue);
    keyboard.println(stringToPrint);
    alert(Blue, 50, 4);
}

/**
 * Checks whether 30 seconds (or whatever the configured awake time is) has passed since the
 * last tag was read. If so, the neopixel is flashed blue 5 times to let the user know that
 * the device is about to go to sleep. It then goes into deep sleep mode, and is awakened
 * by the configured button press.
 */
//void checkSleepyTime()
//{
//    currentMillis = millis();
//
//    return;
//
////    if (currentMillis - startMillis >= AWAKE_PERIOD)
////    {
////        sleep();
////    }
//}

void sleep()
{
    // Halt PICC
    rfid.PICC_HaltA();

    // Stop encryption on PCD
    rfid.PCD_StopCrypto1();

    alert(Green, 200, 3);
    digitalWrite(READER_POWER_PIN, LOW);

    /* Uncomment these lines to use a touch-enabled pin for wakeup
     *
     * touchAttachInterrupt(T3, wakeupCallback, TOUCH_THRESHOLD);
     * esp_sleep_enable_touchpad_wakeup();
     */

    rtc_gpio_pulldown_en(BUTTON_PIN);
    // rtc_gpio_hold_en(BUTTON_PIN);
    esp_sleep_enable_ext0_wakeup(BUTTON_PIN, 1);
    esp_deep_sleep_start();
}


// ----------------------------------------------------------------------
// The following functions are used for debugging, and can be removed if
// necessary.
// ----------------------------------------------------------------------

#ifdef DEBUG

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void printHex(byte *buffer, byte bufferSize)
{
    for (byte i = 0; i < bufferSize; i++)
    {
        if (i > 0)
            Serial.print(buffer[i] < 0x10 ? "-0" : "-");

        Serial.print(buffer[i], HEX);
    }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize)
{
    for (byte i = 0; i < bufferSize; i++)
    {
        if (i > 0)
            Serial.print(buffer[i] < 0x10 ? ".0" : ".");

        Serial.print(buffer[i], DEC);
    }
}

#endif
