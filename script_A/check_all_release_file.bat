@echo off

@echo ==============  release文件检查开始 ==============

if NOT exist .\F3_EFM32_MAIN_BOARD_BS\f3_app_ota_test_.bin                (@echo f3_app_ota_test_.bin is not exist!              && pause && exit)
::if NOT exist .\F3_EFM32_MAIN_BOARD_BS\F3_circle_led_module-EFM32.hex      (@echo F3_circle_led_module-EFM32.hex is not exist!    && pause && exit)
::if NOT exist .\F3_EFM32_MAIN_BOARD_BS\F3_finger_module-EFM32.hex          (@echo F3_finger_module-EFM32.hex is not exist!        && pause && exit)
if NOT exist .\F3_EFM32_MAIN_BOARD_BS\F3_lock-EFM32.hex                   (@echo F3_lock-EFM32.hex is not exist!                 && pause && exit)
::if NOT exist .\F3_EFM32_MAIN_BOARD_BS\F3_zigbee_module-EFM32.hex          (@echo F3_zigbee_module-EFM32.hex is not exist!        && pause && exit)


if NOT exist .\F3_EFM32_MAIN_BOARD_BS\add_script\application_header.bin                               (@echo application_header.bin is not exist!                 && pause && exit)
if NOT exist .\F3_EFM32_MAIN_BOARD_BS\add_script\application_header_log.bin                           (@echo application_header_log.bin is not exist!             && pause && exit)
if NOT exist .\F3_EFM32_MAIN_BOARD_BS\add_script\bootloader.bin                                       (@echo bootloader.bin is not exist!                         && pause && exit)
if NOT exist .\F3_EFM32_MAIN_BOARD_BS\add_script\bootloader.hex                                       (@echo bootloader.hex is not exist!                         && pause && exit)
::if NOT exist .\F3_EFM32_MAIN_BOARD_BS\add_script\factory_circle_led_module_application_header.bin     (@echo factory_circle_led_module_application_header.bin is not exist!                 && pause && exit)
::if NOT exist .\F3_EFM32_MAIN_BOARD_BS\add_script\factory_finger_module_application_header.bin         (@echo factory_finger_module_application_header.bin is not exist!                     && pause && exit)
::if NOT exist .\F3_EFM32_MAIN_BOARD_BS\add_script\factory_zigbee_board_module_application_header.bin   (@echo factory_zigbee_board_module_application_header.bin is not exist!               && pause && exit)


@echo ==============  release文件检查正常 ==============

::pause;

@echo on
