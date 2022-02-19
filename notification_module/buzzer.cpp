/***
 *  Buzzer Driver
 * 
 *  M.Hefny FEB 2022
 * */
#include <iostream>

#include "../helpers/colors.hpp"
#include "../helpers/util_rpi.hpp"

#include "../hal_linux/rpi_gpio.hpp"

#include "buzzer.hpp"

#include "../status.hpp"

using namespace notification;

bool CBuzzer::init (const std::vector<PORT_STATUS>& buzzer_pins)
{
    if (buzzer_pins.size() == 0) 
    {
        std::cout << _LOG_CONSOLE_TEXT << "Buzzer " << _INFO_CONSOLE_TEXT << "Disabled" << std::endl; 
        
        m_error = ENUM_Module_Error_Code::ERR_NO_HW_AVAILABLE;
        return false;
    }


    const int rpi_version = helpers::CUtil_Rpi::getInstance().get_rpi_model();
    if (rpi_version == -1) 
    {
        std::cout << std::endl << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Cannot initialize Buzzer GPIO because it is not RPI-Board" << _NORMAL_CONSOLE_TEXT_ << std::endl;

        m_error = ENUM_Module_Error_Code::ERR_INIT_FAILED;

        return false;
    }


    if (!hal_linux::CRPI_GPIO::getInstance().init())
    {
        std::cout << std::endl << _ERROR_CONSOLE_BOLD_TEXT_ << "Error: Could not initialize Buzzer GPIO pins." << _NORMAL_CONSOLE_TEXT_ << std::endl;

        m_error = ENUM_Module_Error_Code::ERR_INIT_FAILED;

        return false;
    }

   
    m_error = ENUM_Module_Error_Code::ERR_NON;

    for (auto pin : m_port_pins)
    {
        std::cout << _SUCCESS_CONSOLE_TEXT_ << "Initalize Buzzer at GPIO " << _INFO_CONSOLE_TEXT << std::to_string(pin.gpio_pin) << std::endl; 
        hal_linux::CRPI_GPIO::getInstance().pinMode(pin.gpio_pin, HAL_GPIO_OUTPUT);
        hal_linux::CRPI_GPIO::getInstance().write(pin.gpio_pin, GPIO_OFF);
    }
    
    return true;
}


void CBuzzer::uninit()
{
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: LEDS Unint" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    if (m_error != ENUM_Module_Error_Code::ERR_NON) return ;

    // Reset pins again to start condition.   
    for (auto pin : m_port_pins)
    {
        std::cout << _SUCCESS_CONSOLE_TEXT_ << "reset PORTS at GPIO " << std::to_string(pin.gpio_pin) << std::endl; 
        hal_linux::CRPI_GPIO::getInstance().write(pin.gpio_pin, GPIO_OFF);
    }
    
    m_error = ENUM_Module_Error_Code::ERR_UNINITIALIZED;
}


// update - updates led according to timed_updated.  Should be called at 50Hz
void CBuzzer::update()
{
    update_pattern_to_play();
    update_playing_pattern();
}

void CBuzzer::update_pattern_to_play()
{
    STATUS &status = STATUS::getInstance();
    
    if ((get_time_usec() & 0xFFFFFFFF) - _pattern_start_time < _pattern_start_interval_time_us) {
        // do not interrupt playing patterns / enforce minumum separation
        return;
    }

    // check if armed status has changed
    if (m_flags.m_fcb_connected != status.is_online())
    {
        m_flags.m_fcb_connected = status.is_online();
        if (m_flags.m_fcb_connected) 
        {
            play_pattern(ARMING_BUZZ);
        }
        else
        {
            play_pattern(SINGLE_BUZZ);
        }
        return;
    }
    
    // // check ekf bad
    // if (_flags.ekf_bad != AP_Notify::flags.ekf_bad) {
    //     _flags.ekf_bad = AP_Notify::flags.ekf_bad;
    //     if (_flags.ekf_bad) {
    //         // ekf bad warning buzz
    //         play_pattern(EKF_BAD);
    //     }
    //     return;
    // }

    // // if vehicle lost was enabled, starting beep
    // if (AP_Notify::flags.vehicle_lost) {
    //     play_pattern(DOUBLE_BUZZ);
    //     return;
    // }

    // // if battery failsafe constantly single buzz
    // if (AP_Notify::flags.failsafe_battery) {
    //     play_pattern(SINGLE_BUZZ);
    //     return;
    // }
}


void CBuzzer::update_playing_pattern()
{
    if (_pattern == 0UL) {
        return;
    }

    const uint32_t now = get_time_usec() & 0xFFFFFFFF;
    const uint32_t delta = now - _pattern_start_time;
    if (delta >= 3200) {
        // finished playing pattern
        on(0,false);
        _pattern = 0UL;
        return;
    }
    const uint32_t bit = delta / 100UL; // each bit is 100ms
    on(0, _pattern & (1U<<(31-bit)));
}

// on - turns the buzzer on or off
void CBuzzer::on(const uint8_t buzzer_index, const bool turn_on)
{
    // return immediately if nothing to do
    if (m_turn_on == turn_on) {
        return;
    }

    // update state
    m_turn_on = turn_on;

    hal_linux::CRPI_GPIO::getInstance().write(m_port_pins[buzzer_index].gpio_pin, m_turn_on? LED_STATUS_ON : LED_STATUS_OFF);
    m_port_pins[buzzer_index].status = turn_on?GPIO_ON:GPIO_OFF;
}

/// play_pattern - plays the defined buzzer pattern
void CBuzzer::play_pattern(const uint32_t pattern)
{
    _pattern = pattern;
    _pattern_start_time = get_time_usec() & 0xFFFFFFFF;
}


void CBuzzer::switchBuzzer(const uint8_t buzzer_index, const bool onOff, const uint32_t tone)
{
    if (m_error != ENUM_Module_Error_Code::ERR_NON) return ;
    
    if (m_port_pins.size()>=buzzer_index) return ;

    play_pattern(tone);
}


