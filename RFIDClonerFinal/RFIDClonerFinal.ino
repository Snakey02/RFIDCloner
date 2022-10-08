/*
   --------------------------------------------------------------------------------------------------------------------
   Example sketch/program showing how to read new NUID from a PICC to serial.
   --------------------------------------------------------------------------------------------------------------------
   This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid

   Example sketch/program showing how to the read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
   Reader on the Arduino SPI interface.

   When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
   then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
   you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
   will show the type, and the NUID if a new card has been detected. Note: you may see "Timeout in communication" messages
   when removing the PICC from reading distance too early.

   @license Released into the public domain.

   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/

#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SS_PIN 10
#define RST_PIN 13

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

// Init array that will store new NUID
byte nuidPICC[4];

String uidString = "";

int greenled = 7;
int orangeled = 8;
int redled = 9;

void setup() {
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  pinMode(greenled, OUTPUT);
  pinMode(orangeled, OUTPUT);
  pinMode(redled, OUTPUT);

  delay(2000);
  rfid.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  Serial.print(F("Using the following key:"));
  String keyString;
  for (byte i = 0; i < sizeof(key); i++)
  {
    Serial.print(key.keyByte[i] < 0x10 ? " 0" : " ");
    Serial.print(key.keyByte[i], HEX);
    keyString.concat(String(key.keyByte[i] < 0x10 ? " 0" : " "));
    keyString.concat(String(key.keyByte[i], HEX));
  }
  Serial.println();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  Serial.println();
  Serial.println("STEP 1: READ UID");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.print("Please Scan Your Card");
  display.display();
}

void loop() {

  while (nuidPICC[0] == 0 && nuidPICC[1] == 0 && nuidPICC[2] == 0 && nuidPICC[3] == 0) {
    readUID();
  }
  delay(1000);
  writeUID();
}

void readUID() {

  digitalWrite(greenled, HIGH);
  digitalWrite(orangeled, LOW);
  digitalWrite(redled, LOW);

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));
    Serial.println(rfid.uid.size);
    // Store NUID into nuidPICC array
    uidString = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
      uidString.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
      uidString.concat(String(rfid.uid.uidByte[i], HEX));
    }
    rfid.PICC_DumpToSerial(&(rfid.uid));
  }
  else Serial.println(F("Card read previously."));

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();


  /**
    Serial.println("Press Y to continue");
    while (true) // remain here until told to break
    {
      if (Serial.available() > 0) // did something come in?
        if (Serial.read() == 'Y') // is that something the char G?
          break;
    }
  */

  delay(100);

  digitalWrite(greenled, LOW);
  digitalWrite(orangeled, HIGH);

  uidString.toUpperCase();
  Serial.print("UID To Write:");
  Serial.print(uidString);
  Serial.println();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.print("UID To Write:");
  display.println(uidString);
  display.display();
  Serial.println();
  Serial.println("STEP 2: WRITE UID");
  delay(100);
}

void writeUID() {

  digitalWrite(orangeled, HIGH);
  digitalWrite(greenled, LOW);
  digitalWrite(redled, LOW);

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle. And if present, select one.
  if ( ! rfid.PICC_IsNewCardPresent() || ! rfid.PICC_ReadCardSerial() || (rfid.uid.uidByte[0] == nuidPICC[0] && rfid.uid.uidByte[1] == nuidPICC[1] && rfid.uid.uidByte[2] == nuidPICC[2] && rfid.uid.uidByte[3] == nuidPICC[3])) {
    delay(50);
    return;
  }

  // Now a card is selected. The UID and SAK is in rfid.uid.

  // Dump UID
  Serial.print(F("Current Card UID:"));
  String uidStringCurrent = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
    uidStringCurrent.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
    uidStringCurrent.concat(String(rfid.uid.uidByte[i], HEX));
  }
  Serial.println();
  uidStringCurrent.toUpperCase();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.print("Current Card UID:");
  display.println(uidStringCurrent);
  display.display();

  // Dump PICC type
  //  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  //  Serial.print(F("PICC type: "));
  //  Serial.print(mfrc522.PICC_GetTypeName(piccType));
  //  Serial.print(F(" (SAK "));
  //  Serial.print(mfrc522.uid.sak);
  //  Serial.print(")\r\n");
  //  if (  piccType != MFRC522::PICC_TYPE_MIFARE_MINI
  //    &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
  //    &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
  //    Serial.println(F("This sample only works with MIFARE Classic cards."));
  //    return;
  //  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  digitalWrite(orangeled, LOW);

  // Set new UID
  if ( rfid.MIFARE_SetUid(nuidPICC, (byte)4, true) ) {
    Serial.println(F("Wrote new UID to card"));
    display.println("Wrote new UID to card");
    digitalWrite(greenled, HIGH);
  }
  else {
    Serial.println(F("ERROR writing new UID to card"));
    display.println("ERROR writing new UID to card");
    digitalWrite(redled, HIGH);
  }

  display.display();

  // Halt PICC and re-select it so DumpToSerial doesn't get confused
  rfid.PICC_HaltA();
  if ( ! rfid.PICC_IsNewCardPresent() || ! rfid.PICC_ReadCardSerial() ) {
    return;
  }

  // Dump the new memory contents
  Serial.println(F("New UID and contents:"));
  rfid.PICC_DumpToSerial(&(rfid.uid));
  Serial.print(F("New Card UID:"));
  String uidStringNew = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
    uidStringNew.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
    uidStringNew.concat(String(rfid.uid.uidByte[i], HEX));
  }
  uidStringNew.toUpperCase();
  Serial.println();
  display.println("New Card UID:");
  display.println(uidStringNew);
  display.display();

  memset(nuidPICC, 0, sizeof(nuidPICC));
  delay(5000);
  Serial.println();
  Serial.println("STEP 1: READ UID");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.print("Please Scan Your Card");
  display.display();
}


/**
   Helper routine to dump a byte array as hex values to Serial.

  void printHexKey(byte * buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    contentKey.concat(String(buffer[i] < 0x10 ? " 0" : " "));
    contentKey.concat(String(buffer[i], HEX));
  }
  }

  void printHex(byte * buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    content.concat(String(buffer[i] < 0x10 ? " 0" : " "));
    content.concat(String(buffer[i], HEX));
  }
  }
*/
