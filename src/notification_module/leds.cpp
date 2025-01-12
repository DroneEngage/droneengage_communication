/***
 *  LED Driver
 * 
 *  M.Hefny FEB 2022
 * */

#include <iostream>

#include "../helpers/colors.hpp"
#include "../helpers/util_rpi.hpp"
#include "../messages.hpp"
#include "../hal_linux/rpi_gpio.hpp"
#include "leds.hpp"


#include "../de_broker/de_modules_manager.hpp"



using namespace notification;


    
bool CLEDs::init ()
{
    
    m_error = ENUM_Module_Error_Code::ERR_NON;

    return true;
}

void CLEDs::uninit()
{
    std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: LEDS Unint" << _NORMAL_CONSOLE_TEXT_ << std::endl;

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
    if ((m_error != ENUM_Module_Error_Code::ERR_NON) ||(m_status.m_exit_me))
     return ;
    
    if (m_status.is_gpio_module_connected())
    {

        if (m_status.is_online())
        {
            m_power_led_on = LED_ON;
        }
        else
        {
            m_power_led_on = !m_power_led_on;
        }
        
        Json_de message =
        {
            {"a", GPIO_ACTION_PORT_WRITE},
            {"n", "power_led"},
            {"v", m_power_led_on}
        };

        
        const std::string cmd = message.dump();
        
        #ifdef DEBUG
        std::cout << "cmd:" << cmd.c_str() << "::: len:" << cmd.length() << std::endl;
        #endif
        
        de::comm::CUavosModulesManager::getInstance().forwardCommandsToModules(TYPE_AndruavMessage_GPIO_ACTION, cmd.c_str(), cmd.length());
    
    }
    else if (m_counter % 3 == 0)
    {
        
    }

    m_counter++;
}

