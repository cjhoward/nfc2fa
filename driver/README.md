# NFC2FA Driver

This directory contains the NFC2FA driver which interfaces with the NFC controller. The driver handles all encryption and decryption of passwords.

## Building

	g++ main.cpp hidapi/windows/hid.c -Ihidapi/hidapi -fpermissive -lsetupapi -o driver