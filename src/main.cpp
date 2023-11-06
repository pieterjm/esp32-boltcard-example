#include <Adafruit_PN532_NTAG424.h>
#ifdef USE_SPI
#include <SPI.h>
#endif
#ifdef USE_I2C
#include <Wire.h>
#endif


// Use this line for a breakout with a software SPI connection (recommended):
#ifdef USE_SPI
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
#endif
#ifdef USE_I2C
Adafruit_PN532 nfc(PN532_SCL,PN532_SDA);
#endif


void setup(void) {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Hello");
  delay(1000);

  Serial.println("Hello");
  delay(1000);

  Serial.println("Hello");
  delay(1000);

  Serial.println("Hello");

  //WiFi.begin(WIFI_SSID,WIFI_PWD);

  Serial.println("Hello");
  
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1) {
      Serial.println("Could not find PN532");
      sleep(1000);
    }
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN53x");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A Card ...");
}



void loop(void) {
  uint8_t success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength; // Length of the UID (4 or 7 bytes depending on ISO14443A
                     // card type)

  // Wait for an NTAG242 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UUID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    Serial.println("Found a tag");
    Serial.print("  UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    if (((uidLength == 7) || (uidLength == 4)) && (nfc.ntag424_isNTAG424())) {
      Serial.println("Found an NTAG424-tag");

      uint8_t buffer[512];
      uint8_t bytesread = nfc.ntag424_ISOReadFile(buffer);
      String lnurlw = String((char *)buffer,bytesread);
      Serial.println("This is what I read from the card: ");
      Serial.println(lnurlw);
    } else {
      Serial.println("This doesn't seem to be an NTAG424 tag. (UUID length != "
                     "7 bytes and UUID length != 4)!");
    }
    // Wait a bit before trying again
    delay(3000);
  }
}
