# NFC2FA Controller Firmware

This directory contains the NFC2FA controller firmware which can be uploaded to a Teensy++ 2.0 or similar device. The firmware takes commands from the NFC2FA driver and reads and writes data to and from NFC tags using a PN532 NFC/RFID module.

## Building & Installation

1. Install the [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html) add-on for Arduino.
1. Copy all of folders inside the [PN532-PN532_HSU](./libs/PN532-PN532_HSU) directory to the Arduino libraries directory.
2. Open [firmware.ino](./firmware.ino) with the Arduino IDE.
3. Ensure the board is set to `Teensy++ 2.0` and the USB type is set to `Raw HID`.
4. Connect a Teensy++ 2.0 and upload the sketch.