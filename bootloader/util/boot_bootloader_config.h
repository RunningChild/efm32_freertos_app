#ifndef _BOOT_BOOTLOADER_CONFIG_H
#define _BOOT_BOOTLOADER_CONFIG_H


#include "em_device.h"

#include "inner_flash_layout.h"
#include "boot_storage.h"

#include "bsp_config.h"

#include "log.h"


/************************************************
 * Static configuration. Do not modify
 ***********************************************/
/* Firmware Status */
#define FW_NOT_VERIFIED                 (0xffffffff)
#define FW_VERIFIED                     (0x55555555)
#define FW_DELETED                      (0x00000000)

#define WORDS_PER_PAGE                  (FLASH_PAGE_SIZE / 4)

#define AES_BLOCKSIZE                   (16)
#define AES_KEYSIZE                     (32)

/* The space reserved for meta information before
 * each firmware image. The vector table will be placed
 * at FIRMWARE_START_ADDRESS + FIRMWARE_HEADER_SIZE. */
#define FIRMWARE_HEADER_SIZE            (0x100)

/* Address of Debug Lock Word. See reference manual */
#define DEBUG_LOCK_WORD                 (0x0FE04000 + (127 * 4))



/**************************************************
 * Bootloader configuration. Modify this to fit
 * the device and application.
 *************************************************/
#define USE_LEUART                      (LOG_USART_DEVICE_USE_LEUART)

/* Configure UART peripheral */
#define UART                            (LOG_USART_DEVICE)
#define UART_CLOCK                      (LOG_UART_CLOCK)

/* Configure pins and location for UART. */
#define RXPORT                          (LOG_USART_RX_PORT)
#define RXPIN                           (LOG_USART_RX_PIN)
#define TXPORT                          (LOG_USART_TX_PORT)
#define TXPIN                           (LOG_USART_TX_PIN)

#define UART_LOC                        (LOG_USART_DEVICE_POS)//(USART_ROUTE_LOCATION_LOC2)

/* The baudrate to use for bootloader */
#if (RELEASE_VERSION_ENABLE == 0)
#define BOOTLOADER_BAUDRATE             (115200)
#else
#define BOOTLOADER_BAUDRATE             (115200)//(9600)
#endif

/* Which pin to pull high to enter 'bootloader mode' */
#define USE_BOOTLOADER_TRIGGER          (false)

#define BOOTLOADER_TRIGGER_PIN          (COMMON_UNUSE_PIN)
#define BOOTLOADER_TRIGGER_PORT         (COMMON_UNUSE_PORT)

/**************************************************
 * Firmware configuration. Modify this section
 * to change location and size of the firmware
 * location(s).
 *************************************************/

/* Set to true if you want to use a temporary storage when
 * uploading a new firmware image. If set to false the
 * old firmware will be overwritten directly */
#define USE_TEMP_STORAGE                (true)



/* Where to place the new firmware image. The boot address will
 * be at FIRMWARE_START_ADDRESS + FIRMWARE_HEADER_SIZE */
#define FIRMWARE_START_ADDRESS          (APP_REGION_START_ADDRESS)//(0x00004000)
#define FIRMWARE_END_ADDRESS            (APP_REGION_END_ADDRESS)//(0x00063fff)

/* Where to to put the temporary storage used when
 * uploading a new firmware image */
#define TEMP_START_ADDRESS              (SWAP_REGION_START_ADDRESS)//(0x00064000)
#define TEMP_END_ADDRESS                (SWAP_REGION_END_ADDRESS)//(0x000c3fff)


/* Firmware header struct. Gives information
 * about the firmware image */
typedef struct _FirmwareHeader
{
    /* Size in bytes of the binary image */
    uint32_t size;

    /* 128 bit hash */
    uint8_t hash[AES_BLOCKSIZE];

    /* Verified field. Will be set to one of the Firmware Status values defined above. */
    uint32_t verified;

    ota_flash_header_t header;

} FirmwareHeader;

#endif
