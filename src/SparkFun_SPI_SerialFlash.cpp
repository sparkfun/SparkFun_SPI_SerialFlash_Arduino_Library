/*
  Functions to erase, read from and write to SPI serial flash such as
  128mb W25Q128JV
  4mbit AT25SF041
  16mbit GD25Q16C
  64mbit AT45DB641E
  32mbit IS25WP032D

  Not designed to work with serial EEPROMs such as:
  Microchip 25xx SPI Bus Serial EEPROM
  ST 25xx SPI Bus EEPROM

  https://github.com/sparkfun/SparkFun_SPI_SerialFlash_Arduino_Library

  Development environment specifics:
  Arduino IDE 1.8.13

  SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).
  The MIT License (MIT)
  Copyright (c) 2021 SparkFun Electronics
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
  do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial
  portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "SparkFun_SPI_SerialFlash.h"

SFE_SPI_FLASH::SFE_SPI_FLASH(void)
{
  // Constructor
}

//Initialize the library. Check that the flash is responding correctly
bool SFE_SPI_FLASH::begin(uint8_t user_CSPin, uint32_t spiPortSpeed, SPIClass &spiPort, uint8_t spiMode)
{
  //Get user settings
  _PIN_FLASH_CS = user_CSPin;
  _spiPort = &spiPort;
  _spiPortSpeed = spiPortSpeed;
  //if (_spiPortSpeed > 8000000)
  //	_spiPortSpeed = 8000000;
  _spiMode = spiMode;

  pinMode(_PIN_FLASH_CS, OUTPUT);     //Make the CS pin an output
  digitalWrite(_PIN_FLASH_CS, HIGH);  //Deselect the flash

  _spiPort->begin(); //Turn on SPI hardware

  return (isConnected()); //Check that the flash is responding correctly
}

//Check that the flash is responding correctly
//If known manufacturer, then set device type. This affects how status reads work.
bool SFE_SPI_FLASH::isConnected()
{
  sfe_flash_manufacturer_e mfgID = getManufacturerID();
  if (mfgID == SFE_FLASH_MFG_WINBOND) //Winbond
  {
    //25 series
    //https://cdn.sparkfun.com/assets/9/d/7/a/d/w25q128jv_revf_03272018_plus.pdf
    //0x4018 for W25Q128JV-IQ/JQ = 128Mb
    //0x7018 for W25Q128JV-IM* / JM*
    return (true);
  }
  else if (mfgID == SFE_FLASH_MFG_ADESTO) //Adesto
  {
    //0x8401 = 25 series, 4Mb
    //0x2800 = 45 series, 64Mb
    //Set family to either 25xx or 45xx
    uint16_t familyCode = getDeviceID() & 0xFF00;
    familyCode >>= 13; //Bits 5:7 in byte 1

    if (familyCode == 0b100) _flashFamily = SFE_FLASH_FAMILY_25XX; //https://www.adestotech.com/wp-content/uploads/DS-AT25SF041_044.pdf
    else if (familyCode == 0b001) _flashFamily = SFE_FLASH_FAMILY_45XX; //https://www.adestotech.com/wp-content/uploads/DS-45DB641E-027.pdf
    return (true);
  }
  else if (mfgID == SFE_FLASH_MFG_MACRONIX) //Macronix
  {
    //25 series
    //https://www.macronix.com/Lists/Datasheet/Attachments/7363/MX25L1006E%2c%203V%2c%201Mb%2c%20v1.4.pdf
    return (true);
  }
  else if (mfgID == SFE_FLASH_MFG_GIGADEVICE) //GigaDevice
  {
    //25 series
    //0x2011 = 1Mb
    //http://www.gigadevice.com/datasheet/gd25q16c/
    return (true);
  }
  else if (mfgID == SFE_FLASH_MFG_ISSI) //ISSI
  {
    //25 series
    //0x6016 = 32Mb
    //http://www.issi.com/WW/pdf/25LP-WP032D.pdf
    return (true);
  }
  else if (mfgID == SFE_FLASH_MFG_MICROCHIP) //Microchip/SST
  {
    //25 series
    //0x258C = 2Mb
    //https://ww1.microchip.com/downloads/en/DeviceDoc/20005054D.pdf
    return (true);
  }
  else if (mfgID == SFE_FLASH_MFG_MICRON) //Micron
  {
    //25 series
    //0xBA17 = 64Mb
    //https://media-www.micron.com/-/media/client/global/documents/products/data-sheet/nor-flash/serial-nor/mt25q/die-rev-a/mt25q_qlhs_l_128_aba_0.pdf
    return (true);
  }
  else if (mfgID == SFE_FLASH_MFG_INFINEON) //Cypress aka Infineon
  {
    //25 series
    //0x6017 = 64Mb
    //https://www.cypress.com/file/316661/download
    return (true);
  }

  if (_printDebug == true)
  {
    _debugSerial->print(F("SFE_SPI_FLASH::isConnected: Unknown manufacturer code: 0x"));
    if (mfgID < 0x10) _debugSerial->print(F("0"));
    _debugSerial->println(mfgID, HEX);
  }

  return (false);
}

//Send command to do a full erase of the entire flash space
sfe_flash_read_write_result_e SFE_SPI_FLASH::erase()
{
  if (blockingBusyWait(1000) == false) return (SFE_FLASH_READ_WRITE_FAIL_DEVICE_BUSY); //Wait for device to complete previous actions

  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));

  //Write enable
  /*
  The Write Enable instruction sets the Write Enable Latch (WEL) bit in the Status Register to a
  1. The WEL bit must be set prior to every Page Program, Quad Page Program, Sector Erase, Block
  Erase, Chip Erase, Write Status Register and Erase/Program Security Registers instruction.
  */
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_WRITE_ENABLE); //Sets the WEL bit to 1
  digitalWrite(_PIN_FLASH_CS, HIGH);

  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_CHIP_ERASE); //Do entire chip erase
  digitalWrite(_PIN_FLASH_CS, HIGH);

  _spiPort->endTransaction();

  if (_printDebug == true)
  {
    _debugSerial->println(F("SFE_SPI_FLASH::erase: Erasing entire space"));
  }
  unsigned long startTime = millis();
  while (isBusy() == true)
  {
    if (_printDebug == true)
    {
      _debugSerial->print(F("."));
    }
    for (int x = 0 ; x < 50 ; x++)
    {
      delay(10);
      if (isBusy() == false) break;
    }
  }
  unsigned long stopTime = millis();
  if (_printDebug == true)
  {
    _debugSerial->println(F("\nSFE_SPI_FLASH::erase: Erase complete"));
    _debugSerial->print(F("SFE_SPI_FLASH::erase: Time taken: "));
    _debugSerial->println( ((float)(stopTime - startTime)) / 1000.0, 3);
  }

  return (SFE_FLASH_READ_WRITE_SUCCESS);
}

//Reads a byte from a given location
uint8_t SFE_SPI_FLASH::readByte(uint32_t address, sfe_flash_read_write_result_e *result)
{
  if (blockingBusyWait(100) == false) //Wait for device to complete previous actions
  {
    if (result != NULL)
    {
      *result = SFE_FLASH_READ_WRITE_FAIL_DEVICE_BUSY;
    }
    return (0xBB); // Return booboo (because we have to return something...)
  }

  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));
  //Begin reading
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_READ_DATA); //Read command, no dummy bytes
  _spiPort->transfer(address >> 16); //Address byte MSB
  _spiPort->transfer(address >> 8); //Address byte MMSB
  _spiPort->transfer(address & 0xFF); //Address byte LSB
  uint8_t response = _spiPort->transfer(0xFF); //Read in a byte back from flash
  digitalWrite(_PIN_FLASH_CS, HIGH);
  _spiPort->endTransaction();

  if (result != NULL)
  {
    *result = SFE_FLASH_READ_WRITE_SUCCESS;
  }

  return (response);
}

//Reads a block of bytes into a given array, from a given location
sfe_flash_read_write_result_e SFE_SPI_FLASH::readBlock(uint32_t address, uint8_t * dataArray, uint16_t dataSize)
{
  if (dataSize == 0) // Bail if dataSize is zero
    return(SFE_FLASH_READ_WRITE_ZERO_SIZE);

  if (blockingBusyWait(100) == false) return (SFE_FLASH_READ_WRITE_FAIL_DEVICE_BUSY); //Wait for device to complete previous actions

  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));
  //Begin reading
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_READ_DATA); //Read command, no dummy bytes
  _spiPort->transfer(address >> 16); //Address byte MSB
  _spiPort->transfer(address >> 8); //Address byte MMSB
  _spiPort->transfer(address & 0xFF); //Address byte LSB
  for (uint16_t x = 0 ; x < dataSize ; x++)
    dataArray[x] = _spiPort->transfer(0xFF);
  digitalWrite(_PIN_FLASH_CS, HIGH);
  _spiPort->endTransaction();

  return(SFE_FLASH_READ_WRITE_SUCCESS);
}

//Writes a byte to a specific location
sfe_flash_read_write_result_e SFE_SPI_FLASH::writeByte(uint32_t address, uint8_t thingToWrite)
{
  if (blockingBusyWait(100) == false) return (SFE_FLASH_READ_WRITE_FAIL_DEVICE_BUSY); //Wait for device to complete previous actions

  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));

  //Write enable
  /*
  The Write Enable instruction sets the Write Enable Latch (WEL) bit in the Status Register to a
  1. The WEL bit must be set prior to every Page Program, Quad Page Program, Sector Erase, Block
  Erase, Chip Erase, Write Status Register and Erase/Program Security Registers instruction.
  */
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_WRITE_ENABLE); //Sets the WEL bit to 1
  digitalWrite(_PIN_FLASH_CS, HIGH);

  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_PAGE_PROGRAM); //Byte/Page program
  _spiPort->transfer(address >> 16); //Address byte MSB
  _spiPort->transfer(address >> 8); //Address byte MMSB
  _spiPort->transfer(address & 0xFF); //Address byte LSB
  _spiPort->transfer(thingToWrite); //Data!
  digitalWrite(_PIN_FLASH_CS, HIGH);

  _spiPort->endTransaction();

  return(SFE_FLASH_READ_WRITE_SUCCESS);
}

//Write bytes to a specific location
sfe_flash_read_write_result_e SFE_SPI_FLASH::writeBlock(uint32_t address, uint8_t *dataArray, uint16_t dataSize)
{
  if (dataSize == 0) // Bail if dataSize is zero
    return(SFE_FLASH_READ_WRITE_ZERO_SIZE);

  if (blockingBusyWait(100) == false) return (SFE_FLASH_READ_WRITE_FAIL_DEVICE_BUSY); //Wait for device to complete previous actions

  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));

  //Write enable
  /*
  The Write Enable instruction sets the Write Enable Latch (WEL) bit in the Status Register to a
  1. The WEL bit must be set prior to every Page Program, Quad Page Program, Sector Erase, Block
  Erase, Chip Erase, Write Status Register and Erase/Program Security Registers instruction.
  */
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_WRITE_ENABLE); //Sets the WEL bit to 1
  digitalWrite(_PIN_FLASH_CS, HIGH);

  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_PAGE_PROGRAM); //Byte/Page program
  _spiPort->transfer(address >> 16); //Address byte MSB
  _spiPort->transfer(address >> 8); //Address byte MMSB
  _spiPort->transfer(address & 0xFF); //Address byte LSB

  for (uint16_t x = 0 ; x < dataSize ; x++)
    _spiPort->transfer(dataArray[x]); //Data!

  digitalWrite(_PIN_FLASH_CS, HIGH);
  _spiPort->endTransaction();

  return(SFE_FLASH_READ_WRITE_SUCCESS);
}

//Write bytes to a specific location using Auto Address Increment
//This is how multiple bytes are written to (e.g.) the Microchip SST25VF020B
sfe_flash_read_write_result_e SFE_SPI_FLASH::writeBlockAAI(uint32_t address, uint8_t *dataArray, uint16_t dataSize)
{
  if (dataSize == 0) // Bail if dataSize is zero
    return(SFE_FLASH_READ_WRITE_ZERO_SIZE);

  if (dataSize == 1) // AAI can only write byte pairs. If dataSize is 1, just do a writeByte
    return(writeByte(address, dataArray[0]));

  if (blockingBusyWait(100) == false) return (SFE_FLASH_READ_WRITE_FAIL_DEVICE_BUSY); //Wait for device to complete previous actions

  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));

  //DBSY: Disable SO as RY/BY# Status during AAI Programming. We will poll the busy flag instead.
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_DISABLE_SO_DURING_AAI);
  digitalWrite(_PIN_FLASH_CS, HIGH);

  //Write enable
  /*
  The Write Enable instruction sets the Write Enable Latch (WEL) bit in the Status Register to a
  1. The WEL bit must be set prior to every Page Program, Quad Page Program, Sector Erase, Block
  Erase, Chip Erase, Write Status Register and Erase/Program Security Registers instruction.
  */
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_WRITE_ENABLE); //Sets the WEL bit to 1
  digitalWrite(_PIN_FLASH_CS, HIGH);

  //Write the address and the first two bytes of data
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_AAI_WORD_PROGRAM); //AAI Word program (two bytes)
  _spiPort->transfer(address >> 16); //Address byte MSB
  _spiPort->transfer(address >> 8); //Address byte MMSB
  _spiPort->transfer(address & 0xFF); //Address byte LSB
  _spiPort->transfer(dataArray[0]); //Data!
  _spiPort->transfer(dataArray[1]); //Data!
  digitalWrite(_PIN_FLASH_CS, HIGH);

  //Write the remaining data byte pairs
  uint16_t x;
  for (x = 2 ; x < (dataSize - 1); x += 2)
  {
    digitalWrite(_PIN_FLASH_CS, LOW);
    _spiPort->transfer(SFE_FLASH_COMMAND_AAI_WORD_PROGRAM); //AAI Word program (two bytes)
    _spiPort->transfer(dataArray[x]); //Data!
    _spiPort->transfer(dataArray[x+1]); //Data!
    digitalWrite(_PIN_FLASH_CS, HIGH);
  }

  //WRDI: Write Disable - exit AAI mmode
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_WRITE_DISABLE);
  digitalWrite(_PIN_FLASH_CS, HIGH);

  _spiPort->endTransaction();

  //Check if we still have a single byte to write
  if (x == (dataSize - 1))
    return(writeByte(address + dataSize - 1, dataArray[dataSize - 1]));

  return(SFE_FLASH_READ_WRITE_SUCCESS);
}

//Returns true if the device Busy bit is set
//Of course busy logic is different between 25XX vs 45XX and reverse of each other
bool SFE_SPI_FLASH::isBusy()
{
  if (_flashFamily == SFE_FLASH_FAMILY_25XX)
  {
    //Busy bit is bit 0 of status register 1
    uint8_t status = getStatus1();
    if (status & (1 << 0)) return (true); //1 = device is busy
    return (false);
  }
  else //if (_flashFamily == SFE_FLASH_FAMILY_45XX)
  {
    //Busy bit is bit 7 of byte 2
    uint16_t status = getStatus16();
    if (status & (1 << 15)) return (false); //0 = device is busy
    return (true);
  }
}

//Wait for busy flag to clear
bool SFE_SPI_FLASH::blockingBusyWait(uint16_t maxWait)
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
uint8_t SFE_SPI_FLASH::getStatus1()
{
  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_READ_STATUS_25XX); //Read status byte 1
  uint8_t response = _spiPort->transfer(0xFF); //Get byte 1
  digitalWrite(_PIN_FLASH_CS, HIGH);
  _spiPort->endTransaction();

  return (response);
}

//Returns the two status bytes found in 45xx types of flash.
uint16_t SFE_SPI_FLASH::getStatus16()
{
  uint16_t response = 0;

  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_READ_STATUS_45XX); //Read status bytes
  response |= _spiPort->transfer(0xFF); //Get byte 1
  response <<= 8;
  response |= _spiPort->transfer(0xFF); //Get byte 2
  digitalWrite(_PIN_FLASH_CS, HIGH);
  _spiPort->endTransaction();

  return (response);
}

//Set the write status register in 25xx types of flash. Useful for clearing the Block Protetction bits
sfe_flash_read_write_result_e SFE_SPI_FLASH::setWriteStatusReg1(uint8_t statusByte)
{
  if (blockingBusyWait(100) == false) return (SFE_FLASH_READ_WRITE_FAIL_DEVICE_BUSY); //Wait for device to complete previous actions

  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));

  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_ENABLE_WRITE_STATUS_REG); //Enable status register writing
  digitalWrite(_PIN_FLASH_CS, HIGH);

  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_WRITE_STATUS_REG);
  _spiPort->transfer(statusByte);
  digitalWrite(_PIN_FLASH_CS, HIGH);

  _spiPort->endTransaction();

  return(SFE_FLASH_READ_WRITE_SUCCESS);
}

//Set the write status registers in 25xx types of flash. Useful for clearing the Block Protetction bits
sfe_flash_read_write_result_e SFE_SPI_FLASH::setWriteStatusReg16(uint16_t statusWord)
{
  if (blockingBusyWait(100) == false) return (SFE_FLASH_READ_WRITE_FAIL_DEVICE_BUSY); //Wait for device to complete previous actions

  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));

  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_ENABLE_WRITE_STATUS_REG); //Enable status register writing
  digitalWrite(_PIN_FLASH_CS, HIGH);

  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_WRITE_STATUS_REG);
  _spiPort->transfer(statusWord >> 8);
  _spiPort->transfer(statusWord & 0xFF);
  digitalWrite(_PIN_FLASH_CS, HIGH);

  _spiPort->endTransaction();

  return(SFE_FLASH_READ_WRITE_SUCCESS);
}

//Returns the three Manufacturer ID and Device ID bytes
uint32_t SFE_SPI_FLASH::getJEDEC()
{
  uint32_t jedecID = 0;

  //Begin reading 3 JEDEC bytes:
  //MF7-0, ID15-8, ID7-0
  //MfgID, Device ID Part 1, Device ID Part2
  
  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_READ_JEDEC_ID); //Read manufacturer and device ID
  for (uint8_t x = 0 ; x < 3 ; x++)
  {
    jedecID <<= 8;
    jedecID |= _spiPort->transfer(0xFF); //Manufacturer ID, then Device ID byte 1, then Device ID byte 2
  }
  digitalWrite(_PIN_FLASH_CS, HIGH);
  _spiPort->endTransaction();

  return (jedecID);
}

//Reads the 8-bit Manufacturer ID
sfe_flash_manufacturer_e SFE_SPI_FLASH::getManufacturerID()
{
  uint8_t manuID = getJEDEC() >> 16;

  if (_printDebug == true)
  {
    _debugSerial->print(F("SFE_SPI_FLASH::getManufacturerID: Manu ID: 0x"));
    if (manuID < 0x10) _debugSerial->print(F("0"));
    _debugSerial->println(manuID, HEX);
  }

  switch (manuID)
  {
    case ((uint8_t)SFE_FLASH_MFG_INFINEON):
      return (SFE_FLASH_MFG_INFINEON);
      break;
    case ((uint8_t)SFE_FLASH_MFG_ADESTO):
      return (SFE_FLASH_MFG_ADESTO);
      break;
    case ((uint8_t)SFE_FLASH_MFG_MICRON):
      return (SFE_FLASH_MFG_MICRON);
      break;
    case ((uint8_t)SFE_FLASH_MFG_ISSI):
      return (SFE_FLASH_MFG_ISSI);
      break;
    case ((uint8_t)SFE_FLASH_MFG_MICROCHIP):
      return (SFE_FLASH_MFG_MICROCHIP);
      break;
    case ((uint8_t)SFE_FLASH_MFG_MACRONIX):
      return (SFE_FLASH_MFG_MACRONIX);
      break;
    case ((uint8_t)SFE_FLASH_MFG_GIGADEVICE):
      return (SFE_FLASH_MFG_GIGADEVICE);
      break;
    case ((uint8_t)SFE_FLASH_MFG_WINBOND):
      return (SFE_FLASH_MFG_WINBOND);
      break;
    default:
      return (SFE_FLASH_MFG_UNKNOWN);
      break;
  }

  return (SFE_FLASH_MFG_UNKNOWN); // Just in case. Redundant?
}

//Reads the 8-bit Manufacturer ID
uint8_t SFE_SPI_FLASH::getRawManufacturerID()
{
  uint8_t manuID = getJEDEC() >> 16;

  if (_printDebug == true)
  {
    _debugSerial->print(F("SFE_SPI_FLASH::getRawManufacturerID: Manu ID: 0x"));
    if (manuID < 0x10) _debugSerial->print(F("0"));
    _debugSerial->println(manuID, HEX);
  }

  return (manuID);
}

//Reads the Device ID
uint16_t SFE_SPI_FLASH::getDeviceID()
{
  uint16_t deviceID = getJEDEC() & 0xFFFF;

  if (_printDebug == true)
  {
    _debugSerial->print(F("SFE_SPI_FLASH::getDeviceID: Device ID: 0x"));
    if (deviceID < 0x1000) _debugSerial->print(F("0"));
    if (deviceID < 0x100) _debugSerial->print(F("0"));
    if (deviceID < 0x10) _debugSerial->print(F("0"));
    _debugSerial->println(deviceID, HEX);
  }

  return (deviceID);
}

const char *SFE_SPI_FLASH::manufacturerIDString(sfe_flash_manufacturer_e manufacturer)
{
  switch (manufacturer)
  {
  case SFE_FLASH_MFG_INFINEON:
    return "Cypress/Infineon";
    break;
  case SFE_FLASH_MFG_ADESTO:
    return "Adesto";
    break;
  case SFE_FLASH_MFG_MICRON:
    return "Micron";
    break;
  case SFE_FLASH_MFG_ISSI:
    return "ISSI";
    break;
  case SFE_FLASH_MFG_MICROCHIP:
    return "Microchip/SST";
    break;
  case SFE_FLASH_MFG_MACRONIX:
    return "Macronix";
    break;
  case SFE_FLASH_MFG_GIGADEVICE:
    return "GigaDevice";
    break;
  case SFE_FLASH_MFG_WINBOND:
    return "Winbond";
    break;
  default:
    return "UNKNOWN";
    break;
  }
  return "UNKNOWN"; // Just in case. Redundant?
}

//Disable writing with SFE_FLASH_COMMAND_WRITE_DISABLE
/*
The Write Disable instruction resets the Write Enable Latch (WEL) bit in the Status Register to
a 0. The Write Disable instruction is entered by driving /CS low, shifting the instruction code “04h” into the
DI pin and then driving /CS high. Note that the WEL bit is automatically reset after Power-up and upon
completion of the Write Status Register, Erase/Program Security Registers, Page Program, Quad Page
Program, Sector Erase, Block Erase, Chip Erase and Reset instructions.
*/
sfe_flash_read_write_result_e SFE_SPI_FLASH::disableWrite()
{
  if (blockingBusyWait(100) == false) return (SFE_FLASH_READ_WRITE_FAIL_DEVICE_BUSY); //Wait for device to complete previous actions

  _spiPort->beginTransaction(SPISettings(_spiPortSpeed, MSBFIRST, _spiMode));
  //Write disable
  digitalWrite(_PIN_FLASH_CS, LOW);
  _spiPort->transfer(SFE_FLASH_COMMAND_WRITE_DISABLE); //Sets the WEL bit to 0
  digitalWrite(_PIN_FLASH_CS, HIGH);
  _spiPort->endTransaction();

  return(SFE_FLASH_READ_WRITE_SUCCESS);
}

//Enable or disable helpful debug messages
void SFE_SPI_FLASH::enableDebugging(Stream &debugPort)
{
  _debugSerial = &debugPort; //Grab which port the user wants us to use for debugging
  _printDebug = true; //Should we print the commands we send? Good for debugging
}
void SFE_SPI_FLASH::disableDebugging(void)
{
  _printDebug = false; //Turn off extra print statements
}

