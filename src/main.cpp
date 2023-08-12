#include <Adafruit_PN532_NTAG424.h>
#ifdef USE_SPI
#include <SPI.h>
#endif
#include <HTTPClient.h>
#include <ArduinoJson.h>


// Use this line for a breakout with a software SPI connection (recommended):
#ifdef USE_SPI
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
#endif


void setup(void) {
  WiFi.begin(WIFI_SSID,WIFI_PWD);
  

  Serial.begin(115200);
  while (!Serial)
    delay(10); // for Leonardo/Micro/Zero

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1)
      ; // halt
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


bool make_lnurlw_withdraw(String lnurlw, int amount, String memo = "") {
  if ( ! lnurlw.startsWith("lnurlw://")) {
    Serial.println("This is not an LNURLW");
    return false;
  }
  
  // create URL
  String url = String("https://") + lnurlw.substring(9);
  
  HTTPClient http;
  http.begin(url);
  int statusCode = http.GET();
  if ( statusCode != HTTP_CODE_OK ) {
    Serial.println("HTTP request not accepted for LNURLW");
    return false;
  }

  String payload = http.getString();
  http.end();
  
  DynamicJsonDocument doc(2000);
  DeserializationError error = deserializeJson(doc, payload.c_str());
  if ( error != DeserializationError::Ok ) {
    Serial.println("Failed to parse JSON ");
    return false;
  }

  // extract callback URL and k1
  String callback = doc["callback"].as<String>();
  String k1 = doc["k1"].as<String>();
  String status = doc["status"].as<String>();
  String reason = doc["reason"].as<String>();
  if (( status != NULL ) && status.equals("ERROR")) {
    Serial.println("Error in LNURLW response");
    Serial.println(reason);
    return false;
  }

  if ( callback == NULL ) {
    Serial.println("No callback in LNURLW response");
    return false;
  }

  // create an invoice, for now we ignore the min/max of the card
  http.begin(INVOICE_URL);
  http.addHeader("X-Api-Key",INVOICE_KEY);
  http.addHeader("Content-type","application/json");

  // create invoice data
  doc.clear();
  doc["out"] = false;
  doc["amount"] = amount;
  doc["memo"] = memo.c_str();
  payload.clear();
  serializeJson(doc, payload);

  statusCode = http.POST(payload);
  if ( statusCode != HTTP_CODE_CREATED) {
    Serial.println("Failed to create invoice");
    Serial.println(statusCode);
    Serial.printf("POST payload='%s'\n",payload.c_str());
    return false;
  } 
  payload = http.getString();


  error = deserializeJson(doc, payload.c_str());
  if ( error != DeserializationError::Ok ) {
    Serial.println("Failed to parse invoice ");
    return false;
  }


  String payment_request = doc["payment_request"].as<String>();


  url = callback + "?k1=" + k1 + "&pr=" + payment_request;
  Serial.println(url);            

  http.begin(url);
  statusCode = http.GET();
  if ( statusCode != HTTP_CODE_OK ) {
    Serial.println("HTTP request not accepted for callback");
    return false;
  }

  payload = http.getString();
  http.end();
  Serial.println(payload);

  error = deserializeJson(doc, payload.c_str());
  if ( error != DeserializationError::Ok ) {
    Serial.println("Failed to parse payment result ");
    return false;
  }
 
  status = doc["status"].as<String>();  

  if ( status.equals("OK") ) {
    Serial.println("Payment succesfull");
    return true;
  } else {
    Serial.println("Payment failed");
    return false;
  }

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
      make_lnurlw_withdraw(lnurlw, 21,"Some memo");
    } else {
      Serial.println("This doesn't seem to be an NTAG424 tag. (UUID length != "
                     "7 bytes and UUID length != 4)!");
    }
    // Wait a bit before trying again
    delay(3000);
  }
}
