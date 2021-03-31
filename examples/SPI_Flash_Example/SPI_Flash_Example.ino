/*
  Author: Nathan Seidle
  Created: January 29, 2021
  License: Lemonadeware. Buy me a lemonade (or other) someday.

  This sketch demonstrates how to interact with SPI flash such as the
  128mb W25Q128JV
  4mbit AT25SF041
  16mbit GD25Q16C
  64mbit AT45DB641E
  32mbit IS25WP032D
*/

const byte PIN_FLASH_CS = 8;

#include <SPI.h>

typedef enum FlashManufacturer {
  FLASH_MFG_INFINEON = 0x01, //aka Cypress
  FLASH_MFG_ADESTO = 0x1F,
  FLASH_MFG_MICRON = 0x20,
  FLASH_MFG_ISSI = 0x9D,
  FLASH_MFG_MICROCHIP = 0xBF, //aka SST
  FLASH_MFG_MACRONIX = 0xC2,
  FLASH_MFG_GIGADEVICE = 0xC8,
  FLASH_MFG_WINBOND = 0xEF,
} FlashManufacturer;

void setup()
{
  Serial.begin(115200);
  Serial.println("SparkFun SPI Flash Interface");

  pinMode(PIN_FLASH_CS, OUTPUT);

  SPI.begin();

  if (isConnected() == false)
  {
    Serial.println("SPI Flash not detected. Check wiring. Freezing...");
    while (1);
  }

  Serial.print(F("SPI Flash detected. Manufacturer: "));
  uint8_t mfgID = getManufacturerID();
  if (mfgID == FLASH_MFG_WINBOND) //Winbond
    Serial.println(F("Winbond"));
  else if (mfgID == FLASH_MFG_ADESTO) //Adesto
    Serial.println(F("Adesto"));
  else if (mfgID == FLASH_MFG_MACRONIX) //Macronix
    Serial.println(F("Macronix"));
  else if (mfgID == FLASH_MFG_GIGADEVICE) //GigaDevice
    Serial.println(F("GigaDevice"));
  else if (mfgID == FLASH_MFG_ISSI) //ISSI
    Serial.println(F("ISSI"));
  else if (mfgID == FLASH_MFG_MICROCHIP) //Microchip/SST
    Serial.println(F("Microchip/SST"));
  else if (mfgID == FLASH_MFG_MICRON) //Micron
    Serial.println(F("Micron"));
  else if (mfgID == FLASH_MFG_INFINEON) //Cypress aka Infineon
    Serial.println(F("Cypress/Infineon"));
  else
  {
    Serial.print(F("Unknown manufacturer ID: 0x"));
    Serial.println(mfgID, HEX);
  }

  Serial.print(F("Device ID: 0x"));
  Serial.println(getDeviceID(), HEX);

  //Test
  //  char myVal[10] = "Test here";
  //  writeBlock(0x02, myVal, 10); //Address, pointer, size
}

void loop()
{
  Serial.println();
  Serial.println("r)ead HEX values, 1k");
  Serial.println("v)iew ASCII values, 100 bytes");
  Serial.println("d)ump raw values, 1k bytes");
  Serial.println();

  while (Serial.available()) Serial.read();
  while (Serial.available() == 0);

  byte choice = Serial.read();

  if (choice == 'r')
  {
    Serial.println("Read raw values");

    for (int x = 0 ; x < 0x3B00 ; x++)
    {
      if (x % 16 == 0) {
        Serial.println();
        Serial.print("0x");
        if (x < 0x10) Serial.print("0");
        Serial.print(x, HEX);
        Serial.print(": ");
      }

      byte val = readByte(x);
      if (val < 0x10) Serial.print("0");
      Serial.print(val, HEX);
      Serial.print(" ");
    }

    Serial.println();

  }
  else if (choice == 'v')
  {
    Serial.println("View ASCII for 10000 bytes");

    for (int x = 0 ; x < 0x3B00 ; x++)
    {
      if (x % 16 == 0) Serial.println();

      byte val = readByte(x);
      if (isAlphaNumeric(val))
        Serial.write(val);
    }
  }
  else if (choice == 'd')
  {
    for (int x = 0 ; x < 0x3B00 ; x++)
    {
      byte val = readByte(x);
      Serial.write(val);
    }
  }
  else
  {
    Serial.print("Unknown: ");
    Serial.write(choice);
    Serial.println();
  }
}
