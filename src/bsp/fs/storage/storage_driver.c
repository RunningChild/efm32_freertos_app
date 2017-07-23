#include "em_msc.h"
#include "em_int.h"

#include "storage_driver.h"

static volatile bool is_msc_running = false;

/**************************************************************************//**
 * @brief storage_is_idle
 *****************************************************************************/
bool storage_is_idle(void)
{
    return (false == is_msc_running);
}

int16_t emu_flash_write_safe(uint32_t *address, void const *data, uint32_t numBytes)
{
    is_msc_running = true;
    INT_Disable();
    int16_t ret = MSC_WriteWord(address, data, numBytes);
    INT_Enable();
    is_msc_running = false;

    return ret;
}

int16_t emu_flash_read_safe(uint32_t *address, void *data, uint32_t numBytes)
{
    uint16_t i;
    uint8_t * page_start_addr;
    uint8_t * read_data_addr;

    is_msc_running = true;
    INT_Disable();

    page_start_addr = (uint8_t *)address;
    read_data_addr = (uint8_t *)data;
    for(i = 0;i < numBytes; i++)
    {
        *(read_data_addr + i) = *(page_start_addr + i);
    }

    INT_Enable();
    is_msc_running = false;

    return 0;
}

int16_t emu_flash_erase_page_safe(uint32_t *startAddress)
{
    is_msc_running = true;
    INT_Disable();
    int16_t ret = MSC_ErasePage(startAddress);
    INT_Enable();
    is_msc_running = false;

    return ret;
}

void emu_flash_init(void)
{
    MSC_Init();
}


