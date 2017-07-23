#include "boot_bootloader.h"

#include "em_device.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "em_msc.h"
#include "em_gpio.h"
#include "em_emu.h"
#include "em_chip.h"
#include "em_int.h"
#include "boot.h"
#include "uart.h"
#include "xmodem.h"
#include "aes.h"
#include "flash.h"
#include "verify.h"
#include "debuglock.h"
#include "boot_bootloader_config.h"

#include "log.h"

/******************************************************************************
 * Sends a message over UART to let the remote end know that we
 * are in bootloader mode and ready to accept commands.
 *****************************************************************************/
void sendWelcomeMessage(void)
{
  BLUART_sendString("EFM32 AES Bootloader\r\n");

#if 0
  BLUART_sendString("Current firmware is...");

  if ( isFirmwareValid() )
  {
    BLUART_sendString("OK\r\n");
  }
  else
  {
    BLUART_sendString("INVALID!\r\n");
  }
#endif

  BLUART_sendString("Debug Lock is...");
  if ( *((uint32_t *)DEBUG_LOCK_WORD) == 0 )
  {
    BLUART_sendString("ENABLED\r\n");
  }
  else
  {
    BLUART_sendString("DISABLED\r\n");
  }

  BLUART_sendString("Please enter a command: \r\n"
                    " h - display this message\r\n"
                    " u - upload encryped firmware\r\n"
                    " v - verify current firmware\r\n"
                    " l - lock debug access\r\n"
                    " r - reset\r\n");
}


/******************************************************************************
 * Enters 'download mode' where the MCU accepts an encrypted firmware
 * over XMODEM-CRC.
 *****************************************************************************/
void enterDownloadMode(void)
{
  /* Disable interrupts when writing to flash. This is necessary on
   * EFM32G devices or a HardFault will occur if an interrupt is triggered
   * while writing to flash */
  INT_Disable();

  /* Starts an XMODEM-CRC download */
  bool status = XMODEM_download();

  if ( status ) {

    BLUART_sendString("Firmware downloaded\r\n");

    /* If temporaray storage is used: first verify the application,
     * then start copying it to the boot area. */
    if ( USE_TEMP_STORAGE )
    {
      BLUART_sendString("Verifying firmware...");

      if ( verifyTempStorage() )
      {
        /* Mark the temporary storage as verified. This allows
         * the bootloader to know it is safe to restart copying
         * the firmware, should the following operations fail */
        markTempAsVerified();//该位可以写入，之前数据全为0xff
        BLUART_sendString("OK\r\n");

        /* Start copying from temp storage to boot region */
        BLUART_sendString("Activating firmware...");

        markFirmwareAsDeleted();//该位可以写入，之前数据全为0xff

        copyFirmwareFromTempStorage();//该函数写入之前会擦除Firmware

        /* Only mark the boot region as verified after everything is copied. */
        markFirmwareAsVerified();

        BLUART_sendString("DONE\r\n");

        //下载且验证完成重启
        NVIC_SystemReset();

      }
      else
      {
        /* Verification of temporary storage failed */
        BLUART_sendString("INVALID\r\n");
      }
    }
    else /* Temporary storage is not used */
    {
      /* Start verifying the firmware */
      BLUART_sendString("Verifying firmware...");

      if ( verifyActiveFirmware() )
      {
        /* Mark the firmware as verified so the bootloader
         * only has to check this field next time */
        markFirmwareAsVerified();
        BLUART_sendString("OK\r\n");

        //下载且验证完成重启
        NVIC_SystemReset();

      }
      else
      {
        /* Verification of firmware failed */
        BLUART_sendString("INVALID\r\n");
      }
    }
  }
  else
  {
    /* Something went wrong during the download. Abort. */
    BLUART_sendString("Firmware download failed!\r\n");
  }

  INT_Enable();
}

/******************************************************************************
 * Enables Debug Lock by clearing the Debug Lock Word and reset the MCU.
 *****************************************************************************/
void enableDebugLock(void)
{
  if ( DEBUGLOCK_lock() ) {
    BLUART_sendString("Debug Lock Word cleared successfully!\r\n"
                      "Please reset MCU to lock debug access\r\n");
  } else {
    BLUART_sendString("Failed to enable Debug Lock\r\n");
  }
}

/******************************************************************************
 * This function waits for commands over UART
 *****************************************************************************/
void commandLoop(void)
{
  uint8_t command = 0;

  sendWelcomeMessage();

  while(1) {

    /* Wait for command */
    command = BLUART_receive();

    switch(command)
    {
    case 'h':
      sendWelcomeMessage();
      break;
    case 'u':
      enterDownloadMode();
      break;
    case 'v':
      BLUART_sendString("Verifying firmware...");
      if ( verifyActiveFirmware() ) {
        BLUART_sendString("OK\r\n");
      } else {
        BLUART_sendString("INVALID\r\n");
      }
      break;
    case 'l':
      BLUART_sendString("Locking debug access...\r\n");
      enableDebugLock();
      break;
    case 'r':
      BLUART_sendString("Reset\r\n");
      NVIC_SystemReset();
      break;

    }
  }
}

/* Interrupt handler only used to wake up the MCU when
 * waiting for the bootloader pin to be pulled low */
void GPIO_EVEN_IRQHandler(void)
{
  GPIO->IFC = GPIO->IF;
}

/* Interrupt handler only used to wake up the MCU when
 * waiting for the bootloader pin to be pulled low */
void GPIO_ODD_IRQHandler(void)
{
  GPIO->IFC = GPIO->IF;
}

/*****************************************************************************
 * Waits in EM3 until the bootloader pin is pulled low. This saves power
 * while waiting to install firmware.
 *****************************************************************************/
void enterLowPowerWait(void)
{
#if (true == USE_BOOTLOADER_TRIGGER)
  /* Enable interrupt on GPIO pin. */
  GPIO_IntConfig(BOOTLOADER_TRIGGER_PORT, BOOTLOADER_TRIGGER_PIN, true, false, true);
  if ( BOOTLOADER_TRIGGER_PIN % 2 == 0 ) {
    NVIC_EnableIRQ(GPIO_EVEN_IRQn);
  } else {
    NVIC_EnableIRQ(GPIO_ODD_IRQn);
  }

  /* Wait in EM3 until the pin is pulled high */
  while ( !GPIO_PinInGet(BOOTLOADER_TRIGGER_PORT, BOOTLOADER_TRIGGER_PIN) ) {
    EMU_EnterEM3(false);
  }

  /* Disable interrupts again */
  GPIO_IntConfig(BOOTLOADER_TRIGGER_PORT, BOOTLOADER_TRIGGER_PIN, false, false, false);
  if ( BOOTLOADER_TRIGGER_PIN % 2 == 0 ) {
    NVIC_DisableIRQ(GPIO_EVEN_IRQn);
  } else {
    NVIC_DisableIRQ(GPIO_ODD_IRQn);
  }
#endif
}

/*****************************************************************************
 * bootloader_check_firmware_valid.
 * 校验Firmware是否有效
 *****************************************************************************/
static bool bootloader_check_firmware_valid(void)
{
    bool firmware_is_valid_flag = false;

    if(isFirmwareValid())
    {
        BLUART_sendString("Verifying firmware...");
        if ( verifyActiveFirmware() )
        {
            BLUART_sendString("OK\r\n");
            firmware_is_valid_flag = true;
        }
        else
        {
            BLUART_sendString("INVALID\r\n");
        }
    }

    return firmware_is_valid_flag;
}

/*****************************************************************************
 * bootloader_check_temp_storage_valid.
 * 校验TempStorage是否有效
 *****************************************************************************/
static bool bootloader_check_temp_storage_valid(void)
{
    bool temp_storage_is_valid_flag = false;

    if(isTempStorageValid())
    {
        BLUART_sendString("Verifying temp storage...");
        if ( verifyTempStorage() )
        {
            BLUART_sendString("OK\r\n");
            temp_storage_is_valid_flag = true;
        }
        else
        {
            BLUART_sendString("INVALID\r\n");
        }
    }

    return temp_storage_is_valid_flag;
}

/*****************************************************************************
 * bootloader_check_temp_storage_ota_flag.
 * 校验TempStorage ota flag是否有效
 *****************************************************************************/
static bool bootloader_check_temp_storage_ota_flag(bool firmware_valid_flag)
{
    bool temp_storage_is_ota_update_flag = false;

    //进入检查CRC标志的条件:
    //1、firmware校验合法
    //2、temp_storage校验合法
    //3、temp_storage标记有效(如果firmware标记有效，则temp_storage version 大于 firmware version)
    if( (firmware_valid_flag)
     && (isTempStorageOtaFlag()) )
    {
        BLUART_sendString("Verifying temp storage ota crc...");
        BLUART_sendString("\r\n");
        if ( verifyTempStorageOtaCrc() )
        {
            BLUART_sendString("OK\r\n");
            temp_storage_is_ota_update_flag = true;
        }
        else
        {
            BLUART_sendString("INVALID\r\n");
        }
    }

    return temp_storage_is_ota_update_flag;
}

/*****************************************************************************
 * Enters 'bootloader mode' where a new encrypted firmware upgrade can be
 * sent over UART.
 *****************************************************************************/
void enterBootloaderMode(void)
{
    FLASH_init();

    /* Enable UART */
    BLUART_init();

#if 0//_by_pbh 进入bootloader升级引导，不进行swap分区的检查
    bool firmware_is_valid_flag = bootloader_check_firmware_valid();
    bool temp_storage_is_valid_flag = bootloader_check_temp_storage_valid();

    /* If the current firmware is invalid, but temp storage is
    * enabled and contains a valid firmware, copy this firmware
    * to boot region immediately.  */
    if ( USE_TEMP_STORAGE && !firmware_is_valid_flag && temp_storage_is_valid_flag )
    {
        copyFirmwareFromTempStorage();
        markFirmwareAsVerified();

        //下载且验证完成重启
        NVIC_SystemReset();
    }
#endif

#if 0//_by_pbh 等待外部引脚时，不进入低功耗
    /* Check if bootloader pin is pulled high. If it is left low
    * enter EM2 to save power. The MCU will wake up when pin is
    * pulled high. */
    if ( !GPIO_PinInGet(BOOTLOADER_TRIGGER_PORT, BOOTLOADER_TRIGGER_PIN) ) {
        enterLowPowerWait();
    }
#endif

    /* Wait for commands over UART */
    commandLoop();
}

/*****************************************************************************
 * bootloader_entry_init.
 *****************************************************************************/
static void bootloader_entry_init(void)
{
    LOG(LEVEL_DEBUG, "FIRMWARE @0x%x @0x%x", FIRMWARE_START_ADDRESS, FIRMWARE_END_ADDRESS);
    LOG(LEVEL_DEBUG, "SWAP @0x%x @0x%x", TEMP_START_ADDRESS, TEMP_END_ADDRESS);

    LOG(LEVEL_SIMPLE, "\r\n");
}

/*****************************************************************************
 * Bootloader entry point.
 *****************************************************************************/
int bootloader_entry(void)
{
    //初始化
    bootloader_entry_init();

    //开始引导
    CMU_ClockEnable(cmuClock_GPIO, true);

    //检查是否强制进入bootloader升级引导
#if (true == USE_BOOTLOADER_TRIGGER)
    GPIO_PinModeSet(BOOTLOADER_TRIGGER_PORT, BOOTLOADER_TRIGGER_PIN, gpioModeInput, 0);

    /* The bootloader pin is left low, boot normally */
    if ( !GPIO_PinInGet(BOOTLOADER_TRIGGER_PORT, BOOTLOADER_TRIGGER_PIN) )
#else
    if ( GPIO_PinInGet(RXPORT, RXPIN) )
#endif
    {
        bool firmware_is_valid_flag = bootloader_check_firmware_valid();
        bool temp_storage_is_valid_flag = bootloader_check_temp_storage_valid();
        bool temp_storage_is_ota_update_flag = bootloader_check_temp_storage_ota_flag(firmware_is_valid_flag);

        //Firmware is valid && temp storage ota_update is valid
        if(true == temp_storage_is_ota_update_flag)
        {
            BLUART_sendString("OTA update, copy...\r\n");

            FLASH_init();
            copyFirmwareFromTempStorage();//该函数写入之前会擦除Firmware
            markFirmwareAsVerified();

            //Application will boot after reset
            NVIC_SystemReset();
        }


        //Firmware is invalid && temp storage is valid
        if( (USE_TEMP_STORAGE)
         && (false == firmware_is_valid_flag)
         && (true == temp_storage_is_valid_flag) )
        {
            BLUART_sendString("Firmware invalid, copy...\r\n");

            FLASH_init();
            copyFirmwareFromTempStorage();//该函数写入之前会擦除Firmware
            markFirmwareAsVerified();

            //Application will boot after reset
            NVIC_SystemReset();
        }


        //Firmware is valid
        if(true == firmware_is_valid_flag)
        {
            BOOT_boot();
        }
        else
        {
            enterBootloaderMode();
        }

    }
    /* The bootloader pin is pulled high, enter bootloader */
    else
    {
        BLUART_sendString("FORCE enter bootloader...\r\n");
        bootloader_check_firmware_valid();

        enterBootloaderMode();
    }

    /* Never reached */
    return 0;
}

