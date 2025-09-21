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
    #if DEBUG
    std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: LEDS Unint" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
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
            if (m_counter % 2)
            {
                m_power_led_on = LED_OFF;
            }
            else
            {
                m_power_led_on = LED_ON;
            }
        }
        else
        {
            m_power_led_on = LED_OFF;
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
        
        de::comm::CUavosModulesManager::getInstance().forwardCommandsToModules(TYPE_AndruavMessage_GPIO_ACTION, message);
    
    }
    else if (m_counter % 3 == 0)
    {
        
    }

    m_counter++;
}

