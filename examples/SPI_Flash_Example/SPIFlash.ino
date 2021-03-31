/*
  Functions to erase and write to SPI serial flash such as
  128mb W25Q128JV
  4mbit AT25SF041
  16mbit GD25Q16C
  64mbit AT45DB641E
  32mbit IS25WP032D

  Manufacturer IDs:
  Winbond: 0xEF
  Adesto: 0x1F
  Macronix: 0xC2
  GigaDevice: 0xC8
  ISSI: 0x9D
  SST aka Microchip: 0xBF
  Micron: 0x20
  Infineon aka Cypress: 0x01

  Not designed to work with serial EEPROMs such as:
  Microchip 25xx SPI Bus Serial EEPROM
  ST 25xx SPI Bus EEPROM
*/

#define FLASH_COMMAND_PAGE_PROGRAM 0x02
#define FLASH_COMMAND_READ_STATUS_25XX 0x05
#define FLASH_COMMAND_READ_STATUS_45XX 0xD7
#define FLASH_COMMAND_WRITE_ENABLE 0x06
#define FLASH_COMMAND_READ_JEDEC_ID 0x9F
#define FLASH_COMMAND_READ_DATA 0x03
#define FLASH_COMMAND_CHIP_ERASE 0xC7


typedef enum FlashFamily {
  FLASH_FAMILY_25XX,
  FLASH_FAMILY_45XX,
} FlashFamily;

FlashFamily flashFamily = FLASH_FAMILY_25XX; //Default but gets set during isConnected

//Send command to do a full erase of the entire flash space
void erase()
{
  //Write enable
  digitalWrite(PIN_FLASH_CS, LOW);
  SPI.transfer(FLASH_COMMAND_WRITE_ENABLE); //Sets the WEL bit to 1
  digitalWrite(PIN_FLASH_CS, HIGH);

  digitalWrite(PIN_FLASH_CS, LOW);
  SPI.transfer(FLASH_COMMAND_CHIP_ERASE); //Do entire chip erase
  digitalWrite(PIN_FLASH_CS, HIGH);

  //Serial.print("Erasing entire space");
  //long startTime = millis();
  while (isBusy() == true)
  {
    //Serial.print(".");
    for (int x = 0 ; x < 50 ; x++)
    {
      delay(10);
      if (isBusy() == false) break;
    }
  }
//  long stopTime = millis();
//  Serial.println("Erase complete");
//  Serial.print("Time taken: ");
//  Serial.println( (stopTime - startTime) / 1000.0, 3);
}

//Reads a byte from a given location
uint8_t readByte(uint32_t address)
{
  if (blockingBusyWait(100) == false) return (0xBB); //TODO: Return error //Wait for device to complete previous actions

  //Begin reading
  digitalWrite(PIN_FLASH_CS, LOW);
  SPI.transfer(FLASH_COMMAND_READ_DATA); //Read command, no dummy bytes
  SPI.transfer(address >> 16); //Address byte MSB
  SPI.transfer(address >> 8); //Address byte MMSB
  SPI.transfer(address & 0xFF); //Address byte LSB
  uint8_t response = SPI.transfer(0xFF); //Read in a byte back from flash
  digitalWrite(PIN_FLASH_CS, HIGH);

  return (response);
}

//Reads a block of bytes into a given array, from a given location
uint8_t readBlock(uint32_t address, uint8_t * dataArray, uint16_t dataSize)
{
  if (blockingBusyWait(100) == false) return (0xBB); //TODO: Return error //Wait for device to complete previous actions

  //Begin reading
  digitalWrite(PIN_FLASH_CS, LOW);
  SPI.transfer(FLASH_COMMAND_READ_DATA); //Read command, no dummy bytes
  SPI.transfer(address >> 16); //Address byte MSB
  SPI.transfer(address >> 8); //Address byte MMSB
  SPI.transfer(address & 0xFF); //Address byte LSB
  for (uint16_t x = 0 ; x < dataSize ; x++)
    dataArray[x] = SPI.transfer(0xFF);
  digitalWrite(PIN_FLASH_CS, HIGH);

  return(true); //TODO Return success
}

//Writes a byte to a specific location
uint8_t writeByte(uint32_t address, uint8_t thingToWrite)
{
  if (blockingBusyWait(100) == false) return (0xBB); //TODO: Return error //Wait for device to complete previous actions

  //Write enable
  digitalWrite(PIN_FLASH_CS, LOW);
  SPI.transfer(FLASH_COMMAND_WRITE_ENABLE); //Sets the WEL bit to 1
  digitalWrite(PIN_FLASH_CS, HIGH);

  //Write enable
  digitalWrite(PIN_FLASH_CS, LOW);
  SPI.transfer(FLASH_COMMAND_PAGE_PROGRAM); //Byte/Page program
  SPI.transfer(address >> 16); //Address byte MSB
  SPI.transfer(address >> 8); //Address byte MMSB
  SPI.transfer(address & 0xFF); //Address byte LSB
  SPI.transfer(thingToWrite); //Data!
  digitalWrite(PIN_FLASH_CS, HIGH);

  return(true); //TODO Return success
}

//Writes a byte to a specific location
uint8_t writeBlock(uint32_t address, uint8_t *dataArray, uint16_t dataSize)
{
  if (blockingBusyWait(100) == false) return (0xBB); //TODO: Return error //Wait for device to complete previous actions

  //Write enable
  digitalWrite(PIN_FLASH_CS, LOW);
  SPI.transfer(FLASH_COMMAND_WRITE_ENABLE); //Sets the WEL bit to 1
  digitalWrite(PIN_FLASH_CS, HIGH);

  //Write enable
  digitalWrite(PIN_FLASH_CS, LOW);
  SPI.transfer(FLASH_COMMAND_PAGE_PROGRAM); //Byte/Page program
  SPI.transfer(address >> 16); //Address byte MSB
  SPI.transfer(address >> 8); //Address byte MMSB
  SPI.transfer(address & 0xFF); //Address byte LSB

  for (uint16_t x = 0 ; x < dataSize ; x++)
    SPI.transfer(dataArray[x]); //Data!

  digitalWrite(PIN_FLASH_CS, HIGH);
  return(true); //TODO: Return sucess  
}

//Returns true if the device Busy bit is set
//Of course busy logic is different between 25XX vs 45XX and reverse of each other
bool isBusy()
{
  if (flashFamily == FLASH_FAMILY_25XX)
  {
    //Busy bit is bit 0 of status register 1
    uint8_t status = getStatus1();
    if (status & (1 << 0)) return (true); //1 = device is busy
    return (false);
  }
  else if (flashFamily == FLASH_FAMILY_45XX)
  {
    //Busy bit is bit 7 of byte 2
    uint16_t status = getStatus16();
    if (status & (1 << 15)) return (false); //0 = device is busy
    return (true);
  }
}

bool blockingBusyWait(uint16_t maxWait) //TODO: Pass this value in with a default of 100
{
  //Wait for device to complete previous actions
  while (isBusy() == true)
  {
    if (maxWait-- == 0) return (false);
    delay(1);
  }
  return (true);
}


//Returns status byte 0 in 25xx types of flash. Useful for BUSY testing.
uint8_t getStatus1()
{
  digitalWrite(PIN_FLASH_CS, LOW);
  SPI.transfer(FLASH_COMMAND_READ_STATUS_25XX); //Read status byte 1
  uint8_t response = SPI.transfer(0xFF); //Get byte 1
  digitalWrite(PIN_FLASH_CS, HIGH);

  return (response);
}

//Returns the two status bytes found in 45xx types of flash.
uint16_t getStatus16()
{
  uint16_t response = 0;

  digitalWrite(PIN_FLASH_CS, LOW);
  SPI.transfer(FLASH_COMMAND_READ_STATUS_45XX); //Read status bytes
  response |= SPI.transfer(0xFF); //Get byte 1
  response <<= 8;
  response |= SPI.transfer(0xFF); //Get byte 2
  digitalWrite(PIN_FLASH_CS, HIGH);

  return (response);
}

//Returns the three Manufacturer ID and Device ID bytes
uint32_t getJEDEC()
{
  uint32_t JEDEC = 0;

  digitalWrite(PIN_FLASH_CS, HIGH);

  //Begin reading 3 JEDEC bytes:
  //MF7-0, ID15-8, ID7-0
  //MfgID, Device ID Part 1, Device ID Part2
  
  digitalWrite(PIN_FLASH_CS, LOW);
  SPI.transfer(FLASH_COMMAND_READ_JEDEC_ID); //Read manufacturer and device ID
  for (uint8_t x = 0 ; x < 3 ; x++)
  {
    JEDEC <<= 8;
    JEDEC |= SPI.transfer(0xFF); //Manufacturer ID, then Device ID byte 1, then Device ID byte 2
  }
  digitalWrite(PIN_FLASH_CS, HIGH);

  return (JEDEC);
}

//Reads the 8-bit Manufacturer ID
FlashManufacturer getManufacturerID()
{
  uint8_t manuID = getJEDEC() >> 16;

//  Serial.print("Manu ID: 0x");
//  Serial.println(manuID, HEX);

  return ((FlashManufacturer)manuID);
}

//Reads the Device ID
uint16_t getDeviceID()
{
  uint16_t deviceID = getJEDEC() & 0xFFFF;

//  Serial.print("Device ID: 0x");
//  Serial.println(deviceID, HEX);

  return (deviceID);
}

//Check that the flash is responding correctly
//If known manufacturer, then set device type. This affects how status reads work.
bool isConnected()
{
  FlashManufacturer mfgID = getManufacturerID();
  if (mfgID == FLASH_MFG_WINBOND) //Winbond
  {
    //25 series
    //https://cdn.sparkfun.com/assets/9/d/7/a/d/w25q128jv_revf_03272018_plus.pdf
    //0x4018 for W25Q128JV-IQ/JQ = 128Mb
    //0x7018 for W25Q128JV-IM* / JM*
    return (true);
  }
  else if (mfgID == FLASH_MFG_ADESTO) //Adesto
  {
    //0x8401 = 25 series, 4Mb
    //0x2800 = 45 series, 64Mb
    //Set family to either 25xx or 45xx
    uint16_t familyCode = getDeviceID() & 0xFF00;
    familyCode >>= 13; //Bits 5:7 in byte 1

    if (familyCode == 0b100) flashFamily = FLASH_FAMILY_25XX; //https://www.adestotech.com/wp-content/uploads/DS-AT25SF041_044.pdf
    else if (familyCode == 0b001) flashFamily = FLASH_FAMILY_45XX; //https://www.adestotech.com/wp-content/uploads/DS-45DB641E-027.pdf
    return (true);
  }
  else if (mfgID == FLASH_MFG_MACRONIX) //Macronix
  {
    //25 series
    //https://www.macronix.com/Lists/Datasheet/Attachments/7363/MX25L1006E%2c%203V%2c%201Mb%2c%20v1.4.pdf
    return (true);
  }
  else if (mfgID == FLASH_MFG_GIGADEVICE) //GigaDevice
  {
    //25 series
    //0x2011 = 1Mb
    //http://www.gigadevice.com/datasheet/gd25q16c/
    return (true);
  }
  else if (mfgID == FLASH_MFG_ISSI) //ISSI
  {
    //25 series
    //0x6016 = 32Mb
    //http://www.issi.com/WW/pdf/25LP-WP032D.pdf
    return (true);
  }
  else if (mfgID == FLASH_MFG_MICROCHIP) //Microchip/SST
  {
    //25 series
    //0x258C = 2Mb
    //https://ww1.microchip.com/downloads/en/DeviceDoc/20005054D.pdf
    return (true);
  }
  else if (mfgID == FLASH_MFG_MICRON) //Micron
  {
    //25 series
    //0xBA17 = 64Mb
    //https://media-www.micron.com/-/media/client/global/documents/products/data-sheet/nor-flash/serial-nor/mt25q/die-rev-a/mt25q_qlhs_l_128_aba_0.pdf
    return (true);
  }
  else if (mfgID == FLASH_MFG_INFINEON) //Cypress aka Infineon
  {
    //25 series
    //0x6017 = 64Mb
    //https://www.cypress.com/file/316661/download
    return (true);
  }

  //Serial.print(F("Unknown manufacturer code: 0x"));
  //Serial.println(mfgID, HEX);
  return (false);
}
