#include <iostream>

#include "../helpers/colors.hpp"
#include "../helpers/util_rpi.hpp"

#include "../hal_linux/rpi_gpio.hpp"
#include "leds.hpp"

#include "../status.hpp"

#define GPIO_LED_CONNECTION      19
#define GPIO_LED_FLASH           26

#define GPIO_LED_ON     1
#define GPIO_LED_OFF    0

using namespace notification;


void CLEDs::init() 
{
    const int rpi_version = helpers::CUtil_Rpi::getInstance().get_rpi_model();
    if (rpi_version == -1) 
    {
        std::cout << std::endl << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Cannot initialize GPIO because it is not RPI-Board" << _NORMAL_CONSOLE_TEXT_ << std::endl;

        m_error = ENUM_Module_Error_Code::ERR_NO_HW_AVAILABLE;

        return ;
    }

    m_error = ENUM_Module_Error_Code::ERR_NON;

    hal_linux::CRPI_GPIO::getInstance().init();
        
    hal_linux::CRPI_GPIO::getInstance().pinMode(GPIO_LED_CONNECTION, HAL_GPIO_OUTPUT);
    hal_linux::CRPI_GPIO::getInstance().pinMode(GPIO_LED_FLASH, HAL_GPIO_OUTPUT);

    hal_linux::CRPI_GPIO::getInstance().write(GPIO_LED_CONNECTION, GPIO_LED_OFF);
    hal_linux::CRPI_GPIO::getInstance().write(GPIO_LED_FLASH, GPIO_LED_OFF);
}

void CLEDs::update() 
{
    if (m_error != ENUM_Module_Error_Code::ERR_NON) return ;

    if (STATUS::getInstance().is_online())
    {
        hal_linux::CRPI_GPIO::getInstance().write(GPIO_LED_CONNECTION, GPIO_LED_ON);
    }
    else
    {
        hal_linux::CRPI_GPIO::getInstance().toggle(GPIO_LED_CONNECTION);
    }
}

