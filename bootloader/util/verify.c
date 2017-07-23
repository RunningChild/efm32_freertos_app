/**************************************************************************//**
 * @file verify.c
 * @brief Verification of firmware validity
 * @author Silicon Labs
 * @version 1.04
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

#include <string.h>
#include "em_cmu.h"
#include "aes.h"
#include "verify.h"
#include "flash.h"

#include "crc16.h"

#include "log.h"

extern uint8_t hashKey[];
extern uint8_t hashInitVector[];


/******************************************************************************
 * Calculates the hash of the firmware. The hash is the last 128-bit cipher
 * text block in a CBC mode AES encryption.
 *
 * @param firmwareAddress
 *    Address of the firmware location
 *
 * @param hashValue
 *    Buffer to store the hash value. Must be at least 16 bytes wide.
 *****************************************************************************/
static bool calculateHash(uint32_t firmwareAddress, uint8_t *hashValue)
{
  int i;

  FirmwareHeader *fwHeader = (FirmwareHeader *)firmwareAddress;

  /* Check that size is within limits */
  if ( fwHeader->size > FIRMWARE_END_ADDRESS - (FIRMWARE_START_ADDRESS + FIRMWARE_HEADER_SIZE) + 1 ) {
    return false;
  }

  /* Get fixed boot address for firmware */
  uint8_t *addr = (uint8_t *)(firmwareAddress + FIRMWARE_HEADER_SIZE);

  /* Calculate end address of firmware */
  uint8_t *endAddr = (uint8_t *)(firmwareAddress + FIRMWARE_HEADER_SIZE + fwHeader->size);

  /* Buffer holding the last computed cipher text block */
  uint8_t prevCipher[AES_BLOCKSIZE];

  /* Initialize prevCipher with the init vector */
  for ( i=0; i<AES_BLOCKSIZE; i++ ) {
    prevCipher[i] = hashInitVector[i];
  }

  /* Enable the AES clock. This is needed before accessing the AES registers */
  CMU->HFCORECLKEN0 |= CMU_HFCORECLKEN0_AES;

  /* Loop over the entire application */
  while ( addr < endAddr ) {

    /* Encrypt a block in CBC mode. Store output in hashValue */
    encryptCBC128(hashKey, addr, prevCipher, hashValue);

    /* Store the cipher text for next iteration */
    for ( i=0; i<AES_BLOCKSIZE; i++ ) {
      prevCipher[i] = hashValue[i];
    }

    /* Increase the source pointer by one AES block (4 words) */
    addr += AES_BLOCKSIZE;
  }

  /* Disable the AES clock again to save power */
  CMU->HFCORECLKEN0 &= ~CMU_HFCORECLKEN0_AES;

  return true;
}


/******************************************************************************
 * Verifies if a firmware image matches the hash value in the header.
 * If the calculated hash matches the value found in the header
 * this function returns true. Returns false otherwise.
 *****************************************************************************/
bool verifyFirmware(uint32_t firmwareAddress)
{
  int i;
  uint8_t calculatedHash[AES_BLOCKSIZE];
  FirmwareHeader *fwHeader = (FirmwareHeader *)firmwareAddress;

#if 0//log
    LOG(LEVEL_SIMPLE, "\r\n");
    LOG(LEVEL_DEBUG, "Size: %d", fwHeader->size);
    LOG(LEVEL_DEBUG, "Hash: ", fwHeader->hash);
    for(int z = 0; z < AES_BLOCKSIZE; z++)
    {
        LOG(LEVEL_SIMPLE, "%.2x", fwHeader->hash[z]);
    }
    LOG(LEVEL_SIMPLE, "\r\n");
#endif


  /* Calulate hash of the entire application in flash */
  if ( !calculateHash(firmwareAddress, calculatedHash) ) {
    return false;
  }

  /* Compare with the expected value in header */
  for ( i=0; i<AES_BLOCKSIZE; i++ ) {
    if ( calculatedHash[i] != fwHeader->hash[i] ) {
      return false;
    }
  }

  return true;
}

bool verifyActiveFirmware(void)
{
  return verifyFirmware(FIRMWARE_START_ADDRESS);
}

bool verifyTempStorage(void)
{
  return verifyFirmware(TEMP_START_ADDRESS);
}

bool isFirmwareValid(void)
{
  FirmwareHeader *fwHeader = (FirmwareHeader *)FIRMWARE_START_ADDRESS;
  return fwHeader->verified == FW_VERIFIED;
}

bool isTempStorageValid(void)
{
  FirmwareHeader *fwHeader = (FirmwareHeader *)TEMP_START_ADDRESS;
  return fwHeader->verified == FW_VERIFIED;
}

void markFirmwareAsVerified(void)
{
  FirmwareHeader *fwHeader = (FirmwareHeader *)FIRMWARE_START_ADDRESS;
  FLASH_writeWord(&fwHeader->verified, FW_VERIFIED);
}

void markTempAsVerified(void)
{
  FirmwareHeader *fwHeader = (FirmwareHeader *)TEMP_START_ADDRESS;
  FLASH_writeWord(&fwHeader->verified, FW_VERIFIED);
}

void markFirmwareAsDeleted(void)
{
  FirmwareHeader *fwHeader = (FirmwareHeader *)FIRMWARE_START_ADDRESS;
  FLASH_writeWord(&fwHeader->verified, FW_DELETED);
}

/******************************************************************************
 * @brief boot_get_ota_flash_header
 *****************************************************************************/
static void boot_get_ota_flash_header(ota_flash_header_t *header)
{
    FirmwareHeader *fwHeader = (FirmwareHeader *)TEMP_START_ADDRESS;

    *header = fwHeader->header;
}

/*****************************************************************************
 * verifyTempStorageOtaCrc.
 *****************************************************************************/
bool verifyTempStorageOtaCrc(void)
{
    ota_flash_header_t header =
    {
        .ota_update_flag     = 0,
        .ota_update_crc      = 0,
        .ota_update_length   = 0,
        .ota_update_version  = 0,
    };

    boot_get_ota_flash_header(&header);

    uint32_t ota_update_length = header.ota_update_length;
    uint32_t ota_update_crc = header.ota_update_crc;
    uint16_t crc_result = 0;
    int i;

    //检查长度是否合法
    if(ota_update_length >= (WHOLE_APP_MAX_LIMIT_SIZE - FIRMWARE_HEADER_SIZE))
    {
        LOG(LEVEL_DEBUG, "ota update length=%d, limit length=%d, invalid.", ota_update_length, (WHOLE_APP_MAX_LIMIT_SIZE - FIRMWARE_HEADER_SIZE));
        return false;
    }

    //检查CRC是否合法
    uint8_t ff_data[FIRMWARE_HEADER_SIZE] = {0,};
    for (i = 0; i < FIRMWARE_HEADER_SIZE; i++)
        ff_data[i] = 0xFF;

    crc_result = bd_crc16(0x00, (uint8_t *)(TEMP_START_ADDRESS), sizeof(uint32_t) + AES_BLOCKSIZE + sizeof(uint32_t));
    crc_result = bd_crc16(crc_result, ff_data, FIRMWARE_HEADER_SIZE- sizeof(uint32_t) - AES_BLOCKSIZE - sizeof(uint32_t));
    crc_result = bd_crc16(crc_result, (uint8_t *)(TEMP_START_ADDRESS + FIRMWARE_HEADER_SIZE), ota_update_length - FIRMWARE_HEADER_SIZE);

    LOG(LEVEL_DEBUG, "ota update length = 0x%x", ota_update_length);
    LOG(LEVEL_DEBUG, "ota update crc cal = 0x%x, crc need = 0x%x", crc_result, ota_update_crc);

    if( (ota_update_crc == crc_result)
     && (0 != ota_update_crc)
     && (0xFFFF != ota_update_crc) )
    {
        return true;
    }

    return false;
}

/*****************************************************************************
 * isTempStorageOtaFlag.
 *****************************************************************************/
bool isTempStorageOtaFlag(void)
{
    FirmwareHeader *fwHeader = (FirmwareHeader *)FIRMWARE_START_ADDRESS;

    ota_flash_header_t header =
    {
        .ota_update_flag     = 0,
        .ota_update_crc      = 0,
        .ota_update_length   = 0,
        .ota_update_version  = 0,
    };

    boot_get_ota_flash_header(&header);

    //检查ota_update_flag
    if(HEADER_OTA_UPDATE == header.ota_update_flag)
    {
        LOG(LEVEL_DEBUG, "fwHeader ota=0x%x", fwHeader->header.ota_update_flag);
        if(HEADER_OTA_UPDATE == fwHeader->header.ota_update_flag)
        {
            //检查ota_update_version
            LOG(LEVEL_DEBUG, "fwHeader version=0x%x, Header version==0x%x", fwHeader->header.ota_update_version, header.ota_update_version);
            if(fwHeader->header.ota_update_version < header.ota_update_version)
            {
                return true;
            }
        }
        else
        {
            return true;
        }

    }

    return false;
}


