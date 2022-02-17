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


void CLEDs::init (const std::vector<uint8_t>& led_pins)
{
    m_led_pins = led_pins;

    const int rpi_version = helpers::CUtil_Rpi::getInstance().get_rpi_model();
    if (rpi_version == -1) 
    {
        std::cout << std::endl << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Cannot initialize GPIO because it is not RPI-Board" << _NORMAL_CONSOLE_TEXT_ << std::endl;

        m_error = ENUM_Module_Error_Code::ERR_NO_HW_AVAILABLE;

        return ;
    }

    m_error = ENUM_Module_Error_Code::ERR_NON;

    hal_linux::CRPI_GPIO::getInstance().init();

    for (auto pin : m_led_pins)
    {
        std::cout << _SUCCESS_CONSOLE_TEXT_ << "Initalize LED at GPIO " << std::to_string(pin) << std::endl; 
        hal_linux::CRPI_GPIO::getInstance().pinMode(pin, HAL_GPIO_OUTPUT);
        hal_linux::CRPI_GPIO::getInstance().write(pin, GPIO_LED_OFF);
    }

}

void CLEDs::uninit()
{
    if (m_error != ENUM_Module_Error_Code::ERR_NON) return ;

    // Reset pins again to start condition.   
    for (auto pin : m_led_pins)
    {
        std::cout << _SUCCESS_CONSOLE_TEXT_ << "Initalize LED at GPIO " << std::to_string(pin) << std::endl; 
        hal_linux::CRPI_GPIO::getInstance().write(pin, GPIO_LED_OFF);
    }
    
    m_error = ENUM_Module_Error_Code::ERR_UNINITIALIZED;
}


/**
 * @brief returns available LEDS including status LED.
 * 
 * @return int 
 */
int CLEDs::getLEDS() const
{
    return m_led_pins.size();
}

/**
 * @brief Turn On/Off leds
 * 
 * @param led_index 0 means status LED. and cannot be access from this function.
 * @param onOff 
 */
void CLEDs::switchLED(const uint8_t led_index, const bool onOff)
{
    hal_linux::CRPI_GPIO::getInstance().write(led_index, onOff?GPIO_LED_ON:GPIO_LED_OFF);
}

/**
 * @brief called by scheduler to update status LED or leds that toggle.
 * 
 */
void CLEDs::update() 
{
    if (m_error != ENUM_Module_Error_Code::ERR_NON) return ;

    if (STATUS::getInstance().m_exit_me) return ;
    
    if ((STATUS::getInstance().is_online()) && (STATUS::getInstance().is_fcb_connected()))
    {
        hal_linux::CRPI_GPIO::getInstance().write(m_led_pins[0], GPIO_LED_ON);
    }
    else if (!STATUS::getInstance().is_online())
    {
        hal_linux::CRPI_GPIO::getInstance().toggle(m_led_pins[0]);
    }
    else if (m_counter % 3)
    {
        hal_linux::CRPI_GPIO::getInstance().toggle(m_led_pins[0]);
    }

    m_counter++;
}

