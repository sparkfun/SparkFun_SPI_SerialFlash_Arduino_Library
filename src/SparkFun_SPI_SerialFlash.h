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

#ifndef SPARKFUN_SPI_FLASH_LIBRARY_H
#define SPARKFUN_SPI_FLASH_LIBRARY_H

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <SPI.h>

// Flash Commands
typedef enum
{
  SFE_FLASH_COMMAND_WRITE_STATUS_REG = 0x01,        // WRSR
  SFE_FLASH_COMMAND_PAGE_PROGRAM = 0x02,
  SFE_FLASH_COMMAND_READ_DATA = 0x03,
  SFE_FLASH_COMMAND_WRITE_DISABLE = 0x04,           // WRDI
  SFE_FLASH_COMMAND_READ_STATUS_25XX = 0x05,        // RDSR
  SFE_FLASH_COMMAND_WRITE_ENABLE = 0x06,            // WREN
  SFE_FLASH_COMMAND_ENABLE_WRITE_STATUS_REG = 0x50, // EWSR
  SFE_FLASH_COMMAND_ENABLE_SO_DURING_AAI = 0x70,    // EBSY: Enable SO to Output RY/BY# Status during AAI Programming
  SFE_FLASH_COMMAND_DISABLE_SO_DURING_AAI = 0x80,   // DBSY: Disable SO to Output RY/BY# Status during AAI Programming
  SFE_FLASH_COMMAND_READ_JEDEC_ID = 0x9F,
  SFE_FLASH_COMMAND_AAI_WORD_PROGRAM = 0xAD,        // Auto Address Increment Programming
  SFE_FLASH_COMMAND_CHIP_ERASE = 0xC7,
  SFE_FLASH_COMMAND_READ_STATUS_45XX = 0xD7
} sfe_flash_commands_e;

// Flash Family
typedef enum
{
  SFE_FLASH_FAMILY_25XX,
  SFE_FLASH_FAMILY_45XX
} sfe_flash_family_e;

// Flash Manufacturer
typedef enum
{
  SFE_FLASH_MFG_INFINEON = 0x01, //aka Cypress
  SFE_FLASH_MFG_ADESTO = 0x1F,
  SFE_FLASH_MFG_MICRON = 0x20,
  SFE_FLASH_MFG_ISSI = 0x9D,
  SFE_FLASH_MFG_MICROCHIP = 0xBF, //aka SST
  SFE_FLASH_MFG_MACRONIX = 0xC2,
  SFE_FLASH_MFG_GIGADEVICE = 0xC8,
  SFE_FLASH_MFG_WINBOND = 0xEF,
  SFE_FLASH_MFG_UNKNOWN = 0xFF
} sfe_flash_manufacturer_e;

// Read and write result codes
typedef enum
{
  SFE_FLASH_READ_WRITE_FAIL_DEVICE_BUSY = 0,  // Just in case result is cast to boolean
  SFE_FLASH_READ_WRITE_SUCCESS = 1,           // Just in case result is cast to boolean
  SFE_FLASH_READ_WRITE_ZERO_SIZE              // Return this if dataSize is zero
} sfe_flash_read_write_result_e;

class SFE_SPI_FLASH
{

  public:
    SFE_SPI_FLASH(void);
    
    bool begin(uint8_t user_CSPin, uint32_t spiPortSpeed = 2000000, SPIClass &spiPort = SPI, uint8_t spiMode = SPI_MODE0); //Initialize the library. Check that the flash is responding correctly
    bool isConnected(); //Check that the flash is responding correctly
    sfe_flash_read_write_result_e erase(); //Send command to do a full erase of the entire flash space
    uint8_t readByte(uint32_t address, sfe_flash_read_write_result_e *result = NULL); //Reads a byte from a given location
    sfe_flash_read_write_result_e readBlock(uint32_t address, uint8_t *dataArray, uint16_t dataSize); //Reads a block of bytes into a given array, from a given location
    sfe_flash_read_write_result_e writeByte(uint32_t address, uint8_t thingToWrite); //Writes a byte to a specific location
    sfe_flash_read_write_result_e writeBlock(uint32_t address, uint8_t *dataArray, uint16_t dataSize); //Write bytes to a specific location
    sfe_flash_read_write_result_e writeBlockAAI(uint32_t address, uint8_t *dataArray, uint16_t dataSize); //Write bytes to a specific location using Auto Address Increment
    bool isBusy(); //Returns true if the device Busy bit is set
    bool blockingBusyWait(uint16_t maxWait = 100); //Wait for busy flag to clear
    uint8_t getStatus1(); //Returns status byte 0 in 25xx types of flash. Useful for BUSY testing.
    uint16_t getStatus16(); //Returns the two status bytes found in 45xx types of flash.
    sfe_flash_read_write_result_e setWriteStatusReg1(uint8_t statusByte); // Writes statusByte to the Status Register
    sfe_flash_read_write_result_e setWriteStatusReg16(uint16_t statusWord); // Writes statusWord to the Status Register and Status Register 1
    uint32_t getJEDEC(); //Returns the three Manufacturer ID and Device ID bytes
    sfe_flash_manufacturer_e getManufacturerID(); //Reads the 8-bit Manufacturer ID as an enum
    uint8_t getRawManufacturerID(); //Reads the 8-bit Manufacturer ID
    uint16_t getDeviceID(); //Reads the Device ID
    const char *manufacturerIDString(sfe_flash_manufacturer_e manufacturer); //Pretty-print the manufacturer
    sfe_flash_read_write_result_e disableWrite(); //Disable writing with SFE_FLASH_COMMAND_WRITE_DISABLE

    // Enable debug messages using the chosen Serial port (Stream)
    // Boards like the RedBoard Turbo use SerialUSB (not Serial).
    // But other boards like the SAMD51 Thing Plus use Serial (not SerialUSB).
    // These lines let the code compile cleanly on as many SAMD boards as possible.
    #if defined(ARDUINO_ARCH_SAMD)  // Is this a SAMD board?
    #if defined(USB_VID)            // Is the USB Vendor ID defined?
    #if (USB_VID == 0x1B4F)         // Is this a SparkFun board?
    #if !defined(ARDUINO_SAMD51_THING_PLUS) & !defined(ARDUINO_SAMD51_MICROMOD) // If it is not a SAMD51 Thing Plus or SAMD51 MicroMod
    void enableDebugging(Stream &debugPort = SerialUSB); //Given a port to print to, enable debug messages.
    #else
    void enableDebugging(Stream &debugPort = Serial); //Given a port to print to, enable debug messages.
    #endif
    #else
    void enableDebugging(Stream &debugPort = Serial); //Given a port to print to, enable debug messages.
    #endif
    #else
    void enableDebugging(Stream &debugPort = Serial); //Given a port to print to, enable debug messages.
    #endif
    #else
    void enableDebugging(Stream &debugPort = Serial); //Given a port to print to, enable debug messages.
    #endif

    void disableDebugging(void);      //Turn off debug statements

  private:

    sfe_flash_family_e _flashFamily = SFE_FLASH_FAMILY_25XX; //Default but gets set during isConnected

    Stream *_debugSerial;           //The stream to send debug messages to if enabled
    boolean _printDebug = false;    //Flag to print the serial commands we are sending to the Serial port for debug

    SPIClass *_spiPort;             //The generic connection to user's chosen SPI hardware
    unsigned long _spiPortSpeed;    //Optional user defined port speed
    uint8_t _PIN_FLASH_CS;          //The Chip Select pin
    uint8_t _spiMode;               //Use this SPI mode

};

#endif
