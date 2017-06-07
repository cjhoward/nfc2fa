
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
#define MAX_UID_LENGTH 7

PN532_I2C pn532i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532i2c);
unsigned char hidBuffer[64];
int packetSize = 0;
int mode = MODE_PASSIVE;
uint8_t uidRead = 0;
uint8_t uid[MAX_UID_LENGTH];
uint8_t uidLength;

void setup()
{
  Serial.begin(115200);
  Serial.print("Initializing NFC controller... ");
  nfc.begin();
  Serial.print("success\n");
}

void loop()
{
  // Check for commands from driver
  packetSize = RawHID.recv(hidBuffer, 0);
  if (packetSize > 0)
  {
    Serial.print("Received command: ");

    if (hidBuffer[0] & COMMAND_READ)
    {
      Serial.print("READ\n");
      mode = MODE_READ;
    }
    else if (hidBuffer[0] & COMMAND_WRITE)
    {
      Serial.print("WRITE\n");
      mode = MODE_WRITE;
    }
    else
    {
      Serial.print("UNKNOWN\n");
    }
  }

  if (mode == MODE_READ)
  {
    if (nfc.tagPresent())
    {
      NfcTag tag = nfc.read();
      Serial.println(tag.getTagType());
      Serial.print("UID: ");Serial.println(tag.getUidString());
  
      if (tag.hasNdefMessage()) // every tag won't have a message
      {
  
        NdefMessage message = tag.getNdefMessage();
        Serial.print("\nThis NFC Tag contains an NDEF Message with ");
        Serial.print(message.getRecordCount());
        Serial.print(" NDEF Record");
        if (message.getRecordCount() != 1) {
          Serial.print("s");
        }
        Serial.println(".");
        
        int recordCount = message.getRecordCount();
        for (int i = 0; i < recordCount; i++)
        {
          Serial.print("\nNDEF Record ");Serial.println(i+1);
          NdefRecord record = message.getRecord(i);
            
          Serial.print("  TNF: ");Serial.println(record.getTnf());
          Serial.print("  Type: ");Serial.println(record.getType()); // will be "" for TNF_EMPTY
          
          // The TNF and Type should be used to determine how your application processes the payload
          // There's no generic processing for the payload, it's returned as a byte[]
          int payloadLength = record.getPayloadLength();
          byte payload[payloadLength];
          record.getPayload(payload);
  
          // Print the Hex and Printable Characters
          Serial.print("  Payload (HEX): ");
          PrintHexChar(payload, payloadLength);
  
          // Force the data into a String (might work depending on the content)
          // Real code should use smarter processing
          String payloadAsString = "";
          for (int c = 0; c < payloadLength; c++) {
            payloadAsString += (char)payload[c];
          }
          Serial.print("  Payload (as String): ");
          Serial.println(payloadAsString);
  
          // id is probably blank and will return ""
          String uid = record.getId();
          if (uid != "") {
            Serial.print("  ID: ");Serial.println(uid);
          }
        }
      }

      mode = MODE_PASSIVE;
    }
  }
  else if (mode == MODE_WRITE)
  {
    // Detect tag
    if (nfc.tagPresent())
    {
      // Create NDEF record
      NdefMessage message = NdefMessage();
      message.addTextRecord("Hello, World!");
      
      // Write NDEF record to tag
      Serial.print("Writing NDEF record... ");
      bool written = nfc.write(message);
      if (written)
      {
        Serial.print("success\n");        
      }
      else
      {
        Serial.print("failed\n");
      }

      mode = MODE_PASSIVE;
    }
  }
}
