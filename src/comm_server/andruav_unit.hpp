#ifndef ANDRUAV_UNIT_H_
#define ANDRUAV_UNIT_H_

#include <iostream>
#include <string>
#include <map>

#include "../helpers/helpers.hpp"
#include "../helpers/json.hpp"
using Json = nlohmann::json;


namespace uavos
{
    
#define VEHICLE_UNKNOWN     0
#define VEHICLE_TRI         1
#define VEHICLE_QUAD        2
#define VEHICLE_PLANE       3
#define VEHICLE_ROVER       4
#define VEHICLE_HELI        5


#define TelemetryProtocol_No_Telemetry          0
#define TelemetryProtocol_Andruav_Telemetry     1
#define TelemetryProtocol_Mavlink_Telemetry     2
#define TelemetryProtocol_MW_Telemetry          3
#define TelemetryProtocol_DroneKit_Telemetry    4
#define TelemetryProtocol_DJI_Telemetry         5
#define TelemetryProtocol_Unknown_Telemetry     999

#define GPS_MODE_AUTO              0
#define GPS_MODE_MOBILE            1
#define GPS_MODE_FCB               2

#define VEHICLE_GCS        999


/**
 * @brief Hold information for each Andruav Unit
 * 
 */


typedef struct {

    int32_t latitude = 0;
    int32_t longitude = 0;
    int32_t altitude = std::numeric_limits<int32_t>::min();  
    int32_t altitude_relative = std::numeric_limits<int32_t>::min(); 
    float   air_speed = 0.0f;
    float   ground_speed = 0.0f;
    uint64_t last_access_time = 0;
    uint32_t h_acc; /*< [mm] Position uncertainty.*/
    uint32_t v_acc; /*< [mm] Altitude uncertainty.*/
    uint32_t vel_acc; /*< [mm] Speed uncertainty.*/
    uint16_t yaw; /*< [cdeg] Yaw in earth frame from north. Use 0 if this GPS does not provide yaw. Use 65535 if this GPS is configured to provide yaw and is currently unable to provide it. Use 36000 for north.*/

    bool is_valid = false;
    bool is_new = false;   
} ANDRUAV_UNIT_LOCATION;

typedef struct {
  
  bool is_me;
  bool is_gcs;
  bool is_shutdown;
  bool is_flashing;
  bool is_whisling;
  

 // fcb_module
  bool use_fcb;
  bool is_gcs_blocked;
  bool is_armed;
  bool is_flying;
  uint8_t autopilot; // valid if use_fcb = true

  int manual_TX_blocked_mode;
  bool is_tracking_mode;
  int is_video_recording;  // 0 - no recording 1  - one recording .. can be used in uavos cam for multiple recording... 
  int vehicle_type;
  int flying_mode;
  int gps_mode;
  int telemetry_protocol;
  uint64_t flying_total_duration;
  uint64_t flying_last_start_time;

  
  int swarm_leader_formation;
  std::string swarm_leader_I_am_following;
  
  std::string permission;  
  std::string party_id;
  std::string unit_name;
  std::string group_name;
  std::string description;
  uint64_t last_access_time;
  
  bool is_new;      
} ANDRUAV_UNIT_INFO;


class CAndruavUnit
{

    public:


        CAndruavUnit()
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
            m_unit_info.gps_mode                = GPS_MODE_AUTO;
            m_unit_info.telemetry_protocol      = TelemetryProtocol_No_Telemetry;

            m_unit_info.flying_total_duration   = 0;
            m_unit_info.flying_last_start_time  = 0;
            m_unit_info.swarm_leader_I_am_following = std::string("");
            m_unit_info.is_new = true;


        }


        CAndruavUnit (const bool is_me):CAndruavUnit()
        {
           m_unit_info.is_me = is_me;
        }

        CAndruavUnit (const std::string& party_id):CAndruavUnit()
        {
            m_unit_info.is_me = false;
            m_unit_info.party_id = party_id;
            m_unit_info.last_access_time = get_time_usec();
        }


        ANDRUAV_UNIT_INFO& getUnitInfo() 
        {
            return m_unit_info;
        }

        ANDRUAV_UNIT_LOCATION& getUnitLocationInfo()
        {
            return m_unit_location_info;
        }

    protected:
        ANDRUAV_UNIT_INFO m_unit_info;
        ANDRUAV_UNIT_LOCATION m_unit_location_info;
};



class CAndruavUnitMe : public uavos::CAndruavUnit
{

    public:
            static CAndruavUnitMe& getInstance()
            {
                static CAndruavUnitMe instance;

                return instance;
            }

            CAndruavUnitMe(CAndruavUnitMe const&)             = delete;
            void operator=(CAndruavUnitMe const&)             = delete;

    private:
            CAndruavUnitMe();
            
};

/**
 * @brief Handles list of Andruav Units.
 * 
 */
class CAndruavUnits 
{

    public:

            //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
            static CAndruavUnits& getInstance()
            {
                static CAndruavUnits instance;

                return instance;
            }

            CAndruavUnits(CAndruavUnits const&)             = delete;
            void operator=(CAndruavUnits const&)            = delete;

    private:
            CAndruavUnits()
            {

            }
        
    public:
        
        void clearUnits ()
        {
            m_AndruavUnits.clear();
        }

        CAndruavUnit* getUnitByName (const std::string& party_id);
        

        
        void addNewUnit (const std::string& party_id)
        {

        }

    private:
        
        // static std::unique_ptr<
        //                 std::map<std::string, std::unique_ptr<uavos::CAndruavUnit>
        //                 > 
        //                 m_AndruavUnits 
        //     = std::unique_ptr<
        //                 std::map<std::string, std::unique_ptr<uavos::CAndruavUnit>>>
        //                     (new std::map<std::string, std::unique_ptr<uavos::CAndruavUnit>>);

        std::map<std::string, std::unique_ptr<uavos::CAndruavUnit>> m_AndruavUnits;

};


}; // namespace uavos




#endif