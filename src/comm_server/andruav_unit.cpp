#include "../helpers/colors.hpp"
#include "andruav_unit.hpp"





uavos::CAndruavUnitMe::CAndruavUnitMe():uavos::CAndruavUnit(true)
{
    m_unit_info.is_gcs                  = false;
    m_unit_info.is_shutdown             = false;
    m_unit_info.is_flashing             = false;
    m_unit_info.is_whisling             = false;
    m_unit_info.is_video_recording      = false;
    m_unit_info.use_fcb                 = false;
    m_unit_info.is_gcs_blocked          = false;
    m_unit_info.is_armed                = false;
    m_unit_info.is_flying               = false;
    m_unit_info.manual_TX_blocked_mode  = false;

    m_unit_info.vehicle_type            = VEHICLE_UNKNOWN;
    m_unit_info.gps_mode                = GPS_MODE_FCB;
    m_unit_info.telemetry_protocol      = TelemetryProtocol_No_Telemetry;

    m_unit_info.flying_total_duration   = 0;
    m_unit_info.flying_last_start_time  = 0;
    m_unit_info.permission              ="XXXXXXXXXXXX";
    m_unit_info.is_new = false;
}


/**
 * @brief find or create new entry for CAndruavUnit
 * 
 * @param party_id 
 * @return uavos::CAndruavUnit* 
 */
uavos::CAndruavUnit* uavos::CAndruavUnits::getUnitByName (const std::string& party_id)
{
    auto unit = m_AndruavUnits.find(party_id);
    if (unit== m_AndruavUnits.end())
    {
        #ifdef DDEBUG
            std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "getUnitByName " << party_id << " NOT found"<< _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
        uavos::CAndruavUnit * pAndruavUnit= new CAndruavUnit(party_id);
        m_AndruavUnits.insert(std::make_pair(party_id, std::unique_ptr<uavos::CAndruavUnit>(pAndruavUnit)));
                
        auto unit = m_AndruavUnits.find(party_id);
        return unit->second.get();
    }
    else
    {
        #ifdef DDEBUG
            std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "getUnitByName " << party_id << " found"<< _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

        return unit->second.get();
    }
}