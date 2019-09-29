# Login Assistant



## Concept

Incremental build of a keyboard emulation device that automatically types in passwords for accounts based on NFC tags.  Each NFC tag will represent a different account with a different password.



- **Iteration 1 - Introduction to the concept and basic build**
    - Concept
        - Device will type in your password by pretending to be a HID device
        - You ensure that you're in the password field of your login dialog, then le the device enter the password
    - Requirements
        - Device must be able to emulate a keyboard
            - Choices
                - Arduino Leonardo compatible board - Arduino Pro Micro, MellBell Pico, etc
                - AdaFruit Trinket M0 - excellent choice for miniaturized project, but can support only one SPI device, which limits possibilities in future iterations of this project.
                - Raspberry Pi Zero - Can use Python, Java, or Go; GPIO libraries for the components we'll be using is limited; Emulating a HID device is complicated and error prone.
                - ESP8266/ESP32 - not applicable, since they cannot emulate HID devices
            - Selected device - Arduino Pro Micro
        - Small tactile button
        - 1 10K resistor
        - USB cable
    - Build
        - Test keyboard emulation 
        - Simple circuit and sketch to type something when a button is pressed. 
    - Limitations
        - Only a single password is supported
        - Security - anyone who has the the device can plug it in, press the button, and get your password.
        - Password changes require re-programming the device, since the password is basically hard-coded into it.
- **Iteration 2 - Use an RFID/NFC tag instead of a button**
    - Benefits over 1st iteration
        - Ability to support multiple passwords (one per RFID tag)
        - More secure - Mallory must get your RFID tag in order to get your password
        - Cooler than a button
        - Can leave the device attached to your computer
    - Requirements
        - Arduino Pro Micro
        - RC522 RFID reader/writer
        - RFID tag(s)
        - 3.3V voltage regulator
    - Build
        - The RC522 - 3.3V, SPI interface
        - Explain SPI
        - ...
- **Iteration 3 - Store accounts externally on an SD card**
    - Support externally updatable account list on SD card
    - Encrypt passwords
    - Requirements
        - SD card module
        - JSON library
        - XXTEA encryption (or something else)
    - https://randomnerdtutorials.com/decoding-and-encoding-json-with-arduino-or-esp8266/









![Image result for pro micro pinout](images/523a1765757b7f5c6e8b4567.png)