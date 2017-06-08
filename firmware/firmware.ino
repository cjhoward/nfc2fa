
#include <Arduino.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

// Magic numbers which identify packets sent by the host
#define HOST_MAGIC_0 0x2F
#define HOST_MAGIC_1 0xAH
// Magic numbers which identity packets sent by the device
#define DEVICE_MAGIC_0 0x2F
#define DEVICE_MAGIC_1 0xAD
#define COMMAND_READ 0x01
#define COMMAND_WRITE 0x02
#define MESSAGE_TAG_DETECTED 0x01
#define MESSAGE_TAG_READ_SUCCESS 0x02
#define MESSAGE_TAG_READ_FAILURE 0x03
#define MESSAGE_TAG_WRITE_SUCCESS 0x04
#define MESSAGE_TAG_WRITE_FAILURE 0x05
#define MODE_PASSIVE 0x00
#define MODE_READ 0x01
#define MODE_WRITE 0x02
#define MODE_UPLOAD 0x03
#define MAX_UID_LENGTH 7

#define HID_BUFFER_SIZE 64
#define KEY_SIZE 128 // AES-128
#define MAX_KEY_PACKETS (KEY_SIZE / HID_BUFFER_SIZE)


PN532_I2C pn532i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532i2c);
unsigned char hidBuffer[HID_BUFFER_SIZE];
int packetSize = 0;
int mode = MODE_PASSIVE;
uint8_t uidRead = 0;
uint8_t uid[MAX_UID_LENGTH];
uint8_t uidLength;
uint8_t ndefData[KEY_SIZE];
uint8_t ndefPacketCount = 0;
uint16_t passwordLength = 0;
uint8_t passwordPacketCount = 0;
uint16_t currentPasswordByte = 0;

void setup()
{
  Serial.begin(115200);
  Serial.print(F("Initializing NFC controller... "));
  nfc.begin();
  Serial.print(F("success\n"));
}

void loop()
{
  // Check for commands from driver
  packetSize = RawHID.recv(hidBuffer, 0);
  if (packetSize > 0)
  {
    if (mode == MODE_UPLOAD)
    {
      for (int i = 0; i < packetSize; ++i)
      {
        ndefData[currentPasswordByte] = hidBuffer[i];
        ++currentPasswordByte;
      }
      
      ++ndefPacketCount;
      Serial.printf(F("Received key packet %d of %d\n"), ndefPacketCount, passwordPacketCount);

      if (ndefPacketCount == passwordPacketCount)
      {
        mode = MODE_WRITE;
        Serial.print(F("Upload complete. Switching to WRITE mode\n"));
      }
    }
    else
    {
      Serial.print(F("Received command: "));
  
      if (hidBuffer[0] & COMMAND_READ)
      {
        Serial.print(F("READ\n"));
        mode = MODE_READ;
      }
      else if (hidBuffer[0] & COMMAND_WRITE)
      {
        Serial.print(F("WRITE\n"));
        passwordLength = hidBuffer[1] | (hidBuffer[2] << 8);
        Serial.printf("Password length: %d\n", passwordLength);
        passwordPacketCount = passwordLength / HID_BUFFER_SIZE + 1;
        currentPasswordByte = 0;
        mode = MODE_UPLOAD;
        ndefPacketCount = 0;
      }
      else
      {
        Serial.print(F("UNKNOWN\n"));
        mode = MODE_PASSIVE;
      }
    }
  }

  if (mode == MODE_READ)
  {
    if (nfc.tagPresent(10))
    {
      Serial.print(F("Reading tag... "));
      NfcTag tag = nfc.read();
      Serial.print(F("success.\n"));

      if (!tag.hasNdefMessage())
      {
        Serial.print(F("Tag does not contain an NDEF message\n"));
      }
      else
      {
        NdefMessage message = tag.getNdefMessage();

        if (message.getRecordCount() != 1)
        {
          Serial.print(F("Tag does not contain NFC2FA data\n"));
        }
        else
        {
          NdefRecord passwordRecord = message.getRecord(0);

          int payloadLength = passwordRecord.getPayloadLength();
          Serial.printf(F("Payload length: %d\n"), payloadLength);
          Serial.print(F("-----BEGIN KEY-----\n"));
          passwordRecord.getPayload(&ndefData[0]);
          for (int i = 0; i < payloadLength; ++i)
          {
            Serial.printf(F("%02X"), ndefData[i]);
            if ((i + 1) % 16 == 0 || i == payloadLength - 1)
            {
              Serial.print(F("\n"));
            }
            else
            {
              Serial.print(F(" "));
            }
          }
          Serial.print(F("-----END KEY-----\n"));

          packetSize = RawHID.send(ndefData, payloadLength);
        }
      }

      mode = MODE_PASSIVE;
    }
  }
  else if (mode == MODE_WRITE)
  {
    // Detect tag
    if (nfc.tagPresent(10))
    {
      // Create NDEF record
      NdefRecord record = NdefRecord();
      record.setTnf(TNF_UNKNOWN);
      record.setPayload(&ndefData[0], passwordLength);

      // Create NDEF message
      NdefMessage message = NdefMessage();

      // Add NDEF record to NDEF message
      if (!message.addRecord(record))
      {
        Serial.print(F("Failed to add NDEF record to message\n"));
      }      
      
      // Write NDEF message to tag
      Serial.print(F("Writing NDEF record... "));
      bool written = nfc.write(message);
      if (written)
      {
        Serial.print(F("success\n"));        
      }
      else
      {
        Serial.print(F("failed\n"));
      }

      mode = MODE_PASSIVE;
    }
  }
}
