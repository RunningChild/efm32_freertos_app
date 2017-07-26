@echo off

cd ../script_A/


@echo ≤¡≥˝SWAP...
commander device pageerase --range 0x3e000:+0x3a000

JLink.exe -Device EFM32GG230F512 -CommanderScript Keil_Flash_download_APP.jlink

::@echo …’–¥APP...
::commander flash --address 0x4000 application_header.bin

::@echo ∏¥Œª...
::commander device reset


@echo ==============  …’–¥≥…π¶£°==============

pause;

@echo on