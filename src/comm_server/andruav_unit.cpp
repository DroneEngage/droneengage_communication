#include "../helpers/colors.hpp"
#include "andruav_unit.hpp"





de::CAndruavUnitMe::CAndruavUnitMe():de::CAndruavUnit(true)
{
    m_unit_info.is_gcs                  = false;
    m_unit_info.is_shutdown             = false;
    m_unit_info.is_flashing             = false;
    m_unit_info.is_whisling             = false;
    m_unit_info.is_video_recording      = false;
    m_unit_info.use_fcb                 = false;
    m_unit_info.is_gcs_blocked          = false;
    m_unit_info.armed_status            = 0;
    m_unit_info.is_flying               = false;
    m_unit_info.manual_TX_blocked_mode  = false;

    m_unit_info.vehicle_type            = ANDRUAV_UNIT_TYPE::VEHICLE_TYPE_UNKNOWN;
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
 * @return de::CAndruavUnit* 
 */
de::CAndruavUnit* de::CAndruavUnits::getUnitByName (const std::string& party_id)
{
    auto unit = m_AndruavUnits.find(party_id);
    if (unit== m_AndruavUnits.end())
    {
        #ifdef DDEBUG
            std::cout <<__PRETTY_FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "getUnitByName " << party_id << " NOT found"<< _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
        
        de::CAndruavUnit * pAndruavUnit= new CAndruavUnit(party_id);
        auto unit = m_AndruavUnits.emplace(party_id, std::unique_ptr<de::CAndruavUnit>(pAndruavUnit)).first;
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