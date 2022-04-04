/***
 *  Buzzer Driver
 * 
 *  M.Hefny FEB 2022
 * */
#include <iostream>

#include "../helpers/colors.hpp"
#include "../helpers/util_rpi.hpp"
#include "../helpers/helpers.hpp"

#include "../hal_linux/rpi_gpio.hpp"

#include "buzzer.hpp"


using namespace notification;




bool CBuzzer::init (const std::vector<PORT_STATUS>& buzzer_pins)
{
    if (buzzer_pins.size() == 0) 
    {
        std::cout << _LOG_CONSOLE_TEXT << "Buzzer " << _INFO_CONSOLE_TEXT << "Disabled" << std::endl; 
        
        m_error = ENUM_Module_Error_Code::ERR_NO_HW_AVAILABLE;
        return false;
    }

    m_port_pins = buzzer_pins;

    
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

        
        m_buzzer_status.push_back({0UL, 0UL, 0UL, 0UL});
    }
    
    m_status.is_buzzer_connected(true);
   
    return true;
}


void CBuzzer::uninit()
{
    #ifdef DEBUG
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: Buzzer Unint" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

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
    //update_pattern_to_play();
    const std::size_t size = m_buzzer_status.size();
    for (std::size_t i=0; i<size;++i)
    {
        update_playing_pattern(i);
    }
}

void CBuzzer::update_pattern_to_play()
{
    // if ((get_time_usec() & 0xFFFFFFFF) - m_buzzer_status[0].pattern_start_time < _pattern_start_interval_time_us) {
    //     // do not interrupt playing patterns / enforce minumum separation
    //     #ifdef DEBUG
    //         std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: do not interrupt playing patterns / enforce minumum separation:" << std::to_string ((get_time_usec() & 0xFFFFFFFF) - m_buzzer_status[0].pattern_start_time) << _NORMAL_CONSOLE_TEXT_ << std::endl;
    //     #endif
    
    //     return;
    // }

    // check if armed status has changed
    if (m_flags.m_fcb_module_connected != m_status.is_online())
    {
        m_flags.m_fcb_module_connected = m_status.is_online();
        if (m_flags.m_fcb_module_connected) 
        {
            switchBuzzer(0, true, ARMING_BUZZ, 1);
        }
        else
        {
            switchBuzzer(0, true, SINGLE_BUZZ, 1);
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

/**
 * @brief determines next state of buzzer to run a pattern
 * the tone consists of 32 on/off bits determined by a timer.
 * @param buzzer_index 
 */
void CBuzzer::update_playing_pattern(const uint8_t buzzer_index)
{
    #ifdef DEBUG_EXTRA
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: BUZ: update_playing_pattern index:" << std::to_string(buzzer_index) << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif


    if (m_buzzer_status[buzzer_index].tone == 0UL) {
        #ifdef DEBUG_EXTRA
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: BUZ: Null Pattern" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
        return;
    }

    //const uint32_t now = get_time_usec() & 0xFFFFFFFF;
    //const uint32_t delta = now -  m_buzzer_status[buzzer_index].pattern_start_time;
    
    #ifdef DEBUG_EXTRA
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: BUZ: delta " << std::to_string(delta) << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
        
    if (m_buzzer_status[buzzer_index].counter  == 32) {
        // finished playing pattern
        on(buzzer_index,false);
        m_buzzer_status[buzzer_index].counter = 0;
        if (--m_buzzer_status[buzzer_index].repeats ==0)
        {
            m_buzzer_status[buzzer_index].tone = 0UL;
            return;
        }
        else
        {
            switchBuzzer(buzzer_index, true, m_buzzer_status[buzzer_index].tone, m_buzzer_status[buzzer_index].repeats);
            return ;
        }
    }

    m_buzzer_status[buzzer_index].counter ++;
    const uint32_t bit = m_buzzer_status[buzzer_index].counter; // delta / 100UL; // each bit is 100ms
    #ifdef DEBUG_EXTRA
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: bit " << std::to_string(bit) << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
        
    
    on(buzzer_index, m_buzzer_status[buzzer_index].tone & (1U<<(31-bit)));
}

// on - LOW LEVEL turns the buzzer on or off 
void CBuzzer::on(const uint8_t buzzer_index, const bool turn_on)
{
    // // return immediately if nothing to do
    // if ((bool)(m_port_pins[buzzer_index].status) == turn_on) {
    //     #ifdef DEBUG
    //         std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: return immediately if nothing to do " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    //     #endif
    //        return;
    // }

    #ifdef DEBUG_EXTRA
          std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: turn_on " << std::to_string(turn_on) << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    hal_linux::CRPI_GPIO::getInstance().write(m_port_pins[buzzer_index].gpio_pin, turn_on? LED_STATUS_ON : LED_STATUS_OFF);
    m_port_pins[buzzer_index].status = turn_on?GPIO_ON:GPIO_OFF;
}

void CBuzzer::switchBuzzer(const uint8_t buzzer_index, const bool onOff, const uint32_t tone, const uint32_t repeats)
{
    if (m_error != ENUM_Module_Error_Code::ERR_NON) return ;
    
    if (m_port_pins.size()<=buzzer_index) return ;

    if (onOff==false)
    {
        on(buzzer_index, false);
        m_buzzer_status[buzzer_index].tone = 0UL;
        m_buzzer_status[buzzer_index].repeats = 0UL;
        return ;
    }

    m_buzzer_status[buzzer_index].tone = tone;
    m_buzzer_status[buzzer_index].repeats = repeats;
    m_buzzer_status[buzzer_index].pattern_start_time = get_time_usec() & 0xFFFFFFFF;

}


