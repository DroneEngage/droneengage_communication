/***
 *  LED Driver
 * 
 *  M.Hefny FEB 2022
 * */

#include <iostream>

#include "../helpers/colors.hpp"
#include "../helpers/util_rpi.hpp"

#include "../hal_linux/rpi_gpio.hpp"
#include "leds.hpp"






using namespace notification;


    
bool CLEDs::init (const std::vector<PORT_STATUS>& led_pins)
{
    if (led_pins.size() == 0) 
    {
        std::cout << _LOG_CONSOLE_TEXT << "LEDs " << _INFO_CONSOLE_TEXT << "Disabled" << std::endl; 
        
        m_error = ENUM_Module_Error_Code::ERR_NO_HW_AVAILABLE;
        return false;
    }

    m_port_pins = led_pins;

    if (!hal_linux::CRPI_GPIO::getInstance().init())
    {
        std::cout << std::endl << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Could not initialize LED GPIO pins." << _NORMAL_CONSOLE_TEXT_ << std::endl;

        m_error = ENUM_Module_Error_Code::ERR_INIT_FAILED;

        return false;
    }

    m_error = ENUM_Module_Error_Code::ERR_NON;

    for (auto pin : m_port_pins)
    {
        std::cout << _SUCCESS_CONSOLE_TEXT_ << "Initalize LED at GPIO " << _INFO_CONSOLE_TEXT << std::to_string(pin.gpio_pin) << std::endl; 
        hal_linux::CRPI_GPIO::getInstance().pinMode(pin.gpio_pin, HAL_GPIO_OUTPUT);
        hal_linux::CRPI_GPIO::getInstance().write(pin.gpio_pin, GPIO_OFF);
        m_port_pins[pin.gpio_pin].status = GPIO_OFF;
    }
    
    m_status.is_light_connected(true);

    return true;
}

void CLEDs::uninit()
{
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: LEDS Unint" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    if (m_error != ENUM_Module_Error_Code::ERR_NON) return ;

    // Reset pins again to start condition.   
    for (auto pin : m_port_pins)
    {
        std::cout << _SUCCESS_CONSOLE_TEXT_ << "reset LED at GPIO " << std::to_string(pin.gpio_pin) << std::endl; 
        hal_linux::CRPI_GPIO::getInstance().write(pin.gpio_pin, GPIO_OFF);
        m_port_pins[pin.gpio_pin].status = GPIO_OFF;
    }
    
    m_error = ENUM_Module_Error_Code::ERR_UNINITIALIZED;
}


/**
 * @brief Turn On/Off leds
 * 
 * @param led_index 0 means status LED. and cannot be access from this function.
 * @param onOff 
 */
void CLEDs::switchLED(const uint8_t led_index, const bool onOff)
{
    if (m_error != ENUM_Module_Error_Code::ERR_NON) return ;
    
    if (m_port_pins.size()>=led_index) return ;

    hal_linux::CRPI_GPIO::getInstance().write(led_index, onOff?GPIO_ON:GPIO_OFF);
    
    m_port_pins[led_index].status = onOff?LED_STATUS_ON:LED_STATUS_OFF;
}

/**
 * @brief called by scheduler to update status LED or leds that toggle.
 * 
 */
void CLEDs::update() 
{
    if (m_error != ENUM_Module_Error_Code::ERR_NON) return ;
    if (m_status.m_exit_me) return ;
    
    std::cout <<"2" << std::endl;

    if ((m_status.is_online()) && (m_status.is_fcb_module_connected()))
    {
        std::cout <<"3" << std::endl;
        hal_linux::CRPI_GPIO::getInstance().write(m_port_pins[0].gpio_pin, GPIO_ON);
        m_port_pins[0].status = LED_STATUS_ON;
    }
    else if (!m_status.is_online())
    {
        std::cout <<"4" << std::endl;
        hal_linux::CRPI_GPIO::getInstance().toggle(m_port_pins[0].gpio_pin);
        m_port_pins[0].status = LED_STATUS_FLASHING;
    }
    else if (m_counter % 3 == 0)
    {
        std::cout <<"5" << std::endl;
        hal_linux::CRPI_GPIO::getInstance().toggle(m_port_pins[0].gpio_pin);
        m_port_pins[0].status = LED_STATUS_FLASHING;
    }

    std::cout <<"6" << std::endl;
    m_counter++;
}

