#include "boot_hal_manager.h"

#include "em_chip.h"
#include "em_rmu.h"
#include "em_cmu.h"
#include "em_emu.h"

#include "log.h"


/*******************************************************************************
 * @brief hal_init
 ******************************************************************************/
void hal_init(void)
{
    //读取系统硬件复位源
    unsigned long resetCause;

    resetCause = RMU_ResetCauseGet();
    RMU_ResetCauseClear();
    if (resetCause & RMU_RSTCAUSE_WDOGRST)
    {
        LOG(LEVEL_DEBUG, "wdog reset!!!");
    }

}


