/* --------------------------------------------------------------------------------------------------
 *  Temperature and humitity display example
 *
 *  This project displays temperature and humidity sensor data on a 0.96inch OLED display (SSD1306).
 *  The temperature is displayed, by default, in Farenheit, but can be switched between Farenheit and
 *  Celcius by pressing the attached button.  The button uses an interrupt instead of reading the
 *  value in the loop() function, so  that the change can be immediately registered.
 *
 * Uses the following libraries:
 * - AM2320 Sensor Library (https://github.com/asukiaaa/AM2320_asukiaaa)
 *      Provides an easy interface to the AM230 sensor.
 * - U8x8 SSD1306 OLED driver library (https://github.com/olikraus/u8g2)
 *      Display library for the SSD1306 OLED display.  We're going to use the u8x8 part of the library
 *      since it is text-only, faster, and uses much less memory than u8g2.
 * - TaskScheduler(https://github.com/arkhipenko/TaskScheduler)
 *      Implements cooperative multitasking on supported microcontrollers.
 *
 * Pinout
 *  ESP8266 I2C Wiring:
 *      GPIO04 -> SDA
 *      GPIO05 -> SCL
 *  NodeMCU I2C Wiring:
 *      D1 (GPIO04) -> SDA
 *      D2 (GPIO05) -> SCL
 * --------------------------------------------------------------------------------------------------
 * Author:  David Alexis
 * License: MIT License (https://opensource.org/licenses/MIT)
 * --------------------------------------------------------------------------------------------------
 */

#include <U8x8lib.h>
#include <TaskScheduler.h>
#include <ESP8266WiFi.h>
#include <BME280I2C.h>
// #include <EnvironmentCalculations.h>

// Uncomment this line to enable display of debug information to the Serial console.
//#define DEBUG

const static int BUTTON = 12;
const static int DEBOUNCE_DELAY = 500;

// Variables to track sensor data
float temperatureF, temperatureC, humidity, pressure;

// These variables are volatile since they are manipulated from within the
// button interrupt handler.
volatile bool showCelcius = false;
volatile unsigned long lastPressed = 0;

U8X8_SSD1306_128X64_NONAME_HW_I2C display(U8X8_PIN_NONE);

BME280I2C bme;

// We're going to use Tasks to run our sensor reading and display instead of
// using the loop() function.
// Define the callback for the Task.
void sensorCallback();

// Define the sensor reading task
Task sensorTask(2500, TASK_FOREVER, &sensorCallback);
Scheduler taskRunner;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void setup()
{
    #ifdef DEBUG
    Serial.begin(115200);
    while (!Serial);
    Serial.println(F("started"));
    #endif

    WiFi.softAPdisconnect (true);

//    Wire.begin();

    display.begin();
    display.setPowerSave(0);
    display.setContrast(200);   // -- Does not seem to work on the displays I have

    bme.begin();

//    pinMode(BUTTON, INPUT_PULLUP);
//    digitalWrite(BUTTON, HIGH);

    // Set up button to trigger an interrupt.  The button uses a pull-up resistor,
    // so a press is detected by the value going low.  Therefore, the interrupt
    // is set to trigger on FALLING.
//    attachInterrupt(digitalPinToInterrupt(BUTTON), buttonPressed, FALLING);

    displayIcons();
    showCelcius = false;
    lastPressed = millis();

    // Initialize and start our background Task.
    taskRunner.init();
    taskRunner.addTask(sensorTask);
    sensorTask.enable();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void loop()
{
    taskRunner.execute();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sensorCallback()
{
    bme.read(pressure, temperatureC, humidity);
    temperatureF = temperatureC * 1.8 + 32.0;

    #ifdef DEBUG
    Serial.print(F("Humidity: "));
    Serial.print(humidity);
    Serial.print(F("%  Temperature: "));
    Serial.print(temperatureF);
    Serial.print(F("°F"));
    #endif

    displayReadings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void displayReadings()
{
    #ifdef DEBUG
        // Printing the sensor data a comma-separated list enables the values
        // to be displayed nicely as a graph in the serial plotter.
        Serial.print(temperatureC);
        Serial.print(",");
        Serial.println(humidity);
    #endif

    displayTemperature();
    displayHumidity();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void displayIcons()
{
    display.setFont(u8x8_font_open_iconic_weather_2x2);
    display.setCursor(0, 1);
    display.print("E");  // Glyph 69

    display.setFont(u8x8_font_open_iconic_thing_2x2);
    display.setCursor(0, 5);
    display.print('H');  // Glyph 72
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void displayTemperature()
{
    int temperature = (int)(round((showCelcius ? temperatureC : temperatureF) * 10) / 10);

    display.setFont(u8x8_font_courB24_3x4_f);
    display.setCursor(3, 1);
    display.print(temperature);
    display.drawGlyph(9, 1, 176);
    display.setCursor(12, 1);
    display.print(showCelcius ? "C" : "F");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void displayHumidity()
{
    display.setFont(u8x8_font_courR18_2x3_r);
    display.setCursor(3, 5);
    display.print((int)humidity);
    display.print("%  ");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void buttonPressed()
{
    // Handle button debouncing by not handling a button press within a given time from the previous press.
    if ((millis() - lastPressed > DEBOUNCE_DELAY) )
    {
        #ifdef DEBUG
            Serial.println("Button pressed");
        #endif

        showCelcius = !showCelcius;
        lastPressed = millis();
    }
}
