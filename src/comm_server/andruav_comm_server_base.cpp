#include <thread>



#include "../helpers/colors.hpp"
#include "../helpers/helpers.hpp"
#include "../helpers/util_rpi.hpp"
#include "../messages.hpp"


#include "andruav_comm_server_base.hpp"

#include "andruav_facade.hpp"

using namespace de::andruav_servers;

std::thread g1;

/**
 * @brief Disconnect websocket for a time duration
 * 
 * @param on_off 
 * @param duration in seconds
 */
void CAndruavCommServerBase::turnOnOff(const bool on_off, const uint32_t duration_seconds)
{
    m_on_off_delay = duration_seconds;
    if (on_off)
    {
        std::cout << _INFO_CONSOLE_BOLD_TEXT << "WS Module:" << _LOG_CONSOLE_TEXT << " Set Communication Line " << _SUCCESS_CONSOLE_BOLD_TEXT_ <<  " Switched Online" << _LOG_CONSOLE_TEXT <<  " duration (sec): "  << _SUCCESS_CONSOLE_BOLD_TEXT_ << std::to_string(duration_seconds) << _NORMAL_CONSOLE_TEXT_ << std::endl;

        // Create and immediately detach the thread
        g1 = std::thread{[this]() { 
            try
            {
                // Switch online
                m_exit = false;
                if (m_on_off_delay != 0)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(m_on_off_delay));
                    // Switch offline again after delay
                    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " Switched Offline" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
                    uninit(true);
                }
            }
            catch (...)
            {
               std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " EXCEPTION" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
            }
        }};
        g1.detach(); // Detach immediately after creation
    }
    else
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " Switched Offline" << _LOG_CONSOLE_TEXT <<  " duration (sec): " << _SUCCESS_CONSOLE_BOLD_TEXT_ << std::to_string(duration_seconds) << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        CAndruavFacade::getInstance().API_sendCommunicationLineStatus(std::string(), false);
    
        // Create and immediately detach the thread
        g1 = std::thread{[this]() { 
            try
            {
                std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for message to be sent.
                        
                uninit(true);
                    
                if (m_on_off_delay != 0)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(m_on_off_delay));
                    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " Restart" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
                        
                    // re-enable.
                    m_exit = false;
                }
            }
            catch (...)
            {
                std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "WS Module:" << _LOG_CONSOLE_TEXT << "Set Communication Line " << _ERROR_CONSOLE_BOLD_TEXT_ <<  " EXCEPTION" <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
            }
        }};
        g1.detach(); // Detach immediately after creation
    }
}






void CAndruavCommServerBase::switchOnline()
{
    m_exit = false;
}

void CAndruavCommServerBase::switchOffline()
{
    uninit(true);
}