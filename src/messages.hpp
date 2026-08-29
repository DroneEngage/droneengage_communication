

// InterModules command
/**
 * @brief CMD_TYPE_INTERMODULE is used when you want to send a message from a  module to another.
 * This is mainly used to emulate a message comes from an external gcs to a module but is created by another module.
 * i.e. FCB module can emulate take_image even comes from gcs to camera module.
 * Even if you do not use CMD_TYPE_INTERMODULE and uses a command id that is for inter-module commands such as id > 9500 
 * then it will be handled by communicator module such as TYPE_AndruavModule_RemoteExecute.
 */
#define CMD_TYPE_INTERMODULE "uv"
#define CMD_TYPE_SYSTEM_MSG  "s"

// JSON InterModule Fields
#define JSON_INTERMODULE_MODULE_ID              "a"
#define JSON_INTERMODULE_MODULE_CLASS           "b"
#define JSON_INTERMODULE_MODULE_MESSAGES_LIST   "c"
#define JSON_INTERMODULE_MODULE_FEATURES        "d"
#define JSON_INTERMODULE_MODULE_KEY             "e"
#define JSON_INTERMODULE_PARTY_RECORD           "f"
#define JSON_INTERMODULE_SOCKET_STATUS          "g"
#define JSON_INTERMODULE_HARDWARE_ID            "s"
#define JSON_INTERMODULE_HARDWARE_TYPE          "t"
#define JSON_INTERMODULE_VERSION                "v"
#define JSON_INTERMODULE_TIMESTAMP_INSTANCE     "u"
#define JSON_INTERMODULE_RESEND                 "z"



#define HARDWARE_TYPE_CPU   1


/**
 * @brief assume JSON header is never less than 10
 * @details Assume JSON header is never less than 10
 * This is to speed up fiding binary message.
 * @todo Please Confirm.
 */
#define MIN_JSON_HEADER_LEANGTH 10



// Communication Commands

/**
 * @brief Group boradcast
 * @details group broad cast overrides individual. 
 * @see Andruav_Communication_Server for details.
 */
#define CMD_COMM_GROUP                  "g" 
/**
 * @brief Individual broadcast.
 * @details single target except for the following
 * *_GD_* all GCS
 * *_AGN_* all agents
 * @see Andruav_Communication_Server for details.
 */
#define CMD_COMM_INDIVIDUAL             "i" 
    
/**
 * @brief System command.
 * @details this should be handled by communication server. e.g. task access messages.
 * @see Andruav_Communication_Server for details.
 */
#define CMD_COMM_SYSTEM                 "s" 
  

// Reserved Target Values
#define ANDRUAV_PROTOCOL_SENDER_ALL_GCS         "_GCS_"
#define ANDRUAV_PROTOCOL_SENDER_ALL_AGENTS      "_AGN_"
#define ANDRUAV_PROTOCOL_SENDER_ALL             "_GD_"
#define ANDRUAV_PROTOCOL_SENDER_COMM_SERVER     "_SYS_"


// Andruav Protocol Fields
#define ANDRUAV_PROTOCOL_GROUP_ID               "gr"
#define ANDRUAV_PROTOCOL_SENDER                 "sd"
#define ANDRUAV_PROTOCOL_TARGET_ID              "tg"
#define ANDRUAV_PROTOCOL_MESSAGE_TYPE           "mt"
#define ANDRUAV_PROTOCOL_MESSAGE_CMD            "ms"
#define ANDRUAV_PROTOCOL_MESSAGE_PERMISSION     "p"
#define INTERMODULE_ROUTING_TYPE                "ty"
#define INTERMODULE_MODULE_KEY                  "GU"
#define WAITING_EVENT                           "ew"
#define FIRE_EVENT                              "ef"
#define LINKED_TO_STEP                          "ls"




// Andruav Messages

/**
 * @brief
 * la: getLatitude() * 1e-7
 * ln: getLongitude() * 1e-7
 * a: absolute altitude in meter
 * r: relative altitude in meter
        {"y", 0},                                       // yaw in cdeg
        {"3D",0},
        {"SC",0},
        {"p",0}
 */
#define TYPE_AndruavMessage_GPS                     1002
#define TYPE_AndruavMessage_POWER                   1003
#define TYPE_AndruavMessage_ID 	                 1004
/**
 * @brief This command is used to execute a remote command on another unit.
 * @param C command id.
 *      * 1- This command is can me less than 1000 which means it is a specific command need to 
 *        be executed by target drone such as RemoteCommand_REQUEST_PARA_LIST, RemoteCommand_MISSION_COUNT ...etc.
 *      * 2- It also can be equal to a command ID such as TYPE_AndruavMessage_ID
 *        in this case the command need to be sent by the target drone, which in this case wwill
 *        be sendID. TYPE_AndruavMessage_RemoteExecute(TYPE_AndruavMessage_ID) === request ID and 
 *        target drone should reply with TYPE_AndruavMessage_ID == sendID.
 *                   TYPE_AndruavMessage_RemoteExecute(TYPE_AndruavMessage_HomeLocation) == send Home Location
 *       * Not all command s are implemented.
 * @param Act :bool which used with @param C to activate & deactivate as extra parameter.
 * 
 */
#define TYPE_AndruavMessage_RemoteExecute 	   1005
#define TYPE_AndruavMessage_IMG                     1006
#define TYPE_AndruavMessage_Error                   1008    
#define TYPE_AndruavMessage_FlightControl           1010
#define TYPE_AndruavMessage_CameraList 		   1012  //RX: {"tg":"GCS1","sd":"zxcv","ty":"c","gr":"1","cm":"i","mt":1012,"ms":"{\"E\":2,\"P\":0,\"I\":\"zxcv\"}"}
#define TYPE_AndruavMessage_DroneReport             1020
#define TYPE_AndruavMessage_Signaling               1021
#define TYPE_AndruavMessage_HomeLocation            1022
#define TYPE_AndruavMessage_GeoFence                1023
#define TYPE_AndruavMessage_ExternalGeoFence        1024
#define TYPE_AndruavMessage_GEOFenceHit             1025
#define TYPE_AndruavMessage_WayPoints               1027
#define TYPE_AndruavMessage_GeoFenceAttachStatus    1029
#define TYPE_AndruavMessage_Arm                     1030
#define TYPE_AndruavMessage_ChangeAltitude          1031
#define TYPE_AndruavMessage_Land                    1032
#define TYPE_AndruavMessage_GuidedPoint             1033
#define TYPE_AndruavMessage_CirclePoint             1034
#define TYPE_AndruavMessage_DoYAW                   1035
#define TYPE_AndruavMessage_NAV_INFO                1036
#define TYPE_AndruavMessage_DistinationLocation     1037
#define TYPE_AndruavMessage_ConfigCOM               1038
#define TYPE_AndruavMessage_ConfigFCB               1039
#define TYPE_AndruavMessage_ChangeSpeed             1040
#define TYPE_AndruavMessage_Ctrl_Cameras            1041
#define TYPE_AndruavMessage_TrackingTarget_ACTION   1042
#define TYPE_AndruavMessage_TrackingTargetLocation  1043
#define TYPE_AndruavMessage_TrackingTarget_STATUS   1044
#define TYPE_AndruavMessage_UploadWayPoints         1046
#define TYPE_AndruavMessage_RemoteControlSettings   1047
#define TYPE_AndruavMessage_SET_HOME_LOCATION       1048
#define TYPE_AndruavMessage_CameraZoom              1049
#define TYPE_AndruavMessage_CameraSwitch            1050
#define TYPE_AndruavMessage_CameraFlash		   1051
#define TYPE_AndruavMessage_RemoteControl2	   1052
#define TYPE_AndruavMessage_SensorsStatus           1053

/**
 * @brief tell a drone that another drone is in its team -a follower-.
 * @details 
 *  ------------------------------
 *  | Sender | Receiver | Action |  
 *  ------------------------------
 *    (1,3)GCS       Drone(follower)      Ask this drone to be a follower/unfollow a leader
 *    ANY       Drone(other)         This is just an announcement. No action is required.
 *    (2) LEADER    Drone(follower)      Confirm that this drone is a follower. and gives index and formation. requires a confirmation from follower.
*  (1) This message can be sent from GCS or another Drone either a leader or not.
 * (2) This message requests from the receiver "Drone" to send @ref TYPE_AndruavMessage_UpdateSwarm to Leader Drone.
 * The receiver can refuse to send @ref TYPE_AndruavMessage_UpdateSwarm
 * and the third drone can also refuse the request to be followed by the receiver.
 * @note receiver should not assume it is a follower. It only should forward this request to the leader.
 */
#define TYPE_AndruavMessage_FollowHim_Request           1054
/**
 * @brief This message is sent from Leader drone to a follower. It guides it to the destination point that it wants it to go to.
 * @details
 * There is nothing called a Follower Drone
 * All Drones Obey AndruavResala_FollowMe_Guided EVEN if they are Leaders.<br>
 * If a Drone wants to IGNORE these messages that is OK for whatever reason.<br>
 * If a Drone wants to Stop others from sending such messages it can send ANdruavResala_UpdateSwarm with remove action.
 */
#define TYPE_AndruavMessage_FollowMe_Guided             1055
/**
 * @brief This command is sent to instruct a drone to be a leader with a swarm-formation.
 * A Formation FORMATION_SERB_NO_SWARM means there is no swarm mode anymore. 
 */
#define TYPE_AndruavMessage_Make_Swarm                  1056
#define TYPE_AndruavMessage_SwarmReport                 1057
/**
 * @brief This message is sent to Leader Drone to add a slave drone in a swarm and in an index.
 * given index may contradict with other indices. It is upto Leader Drone to handle this conflict.
 */
#define TYPE_AndruavMessage_UpdateSwarm                 1058
#define TYPE_AndruavMessage_CommSignalsStatus           1059
/**
 * d: event-id
 * [c]: sender module class type
 * [s]: sender module class id
 * [m]: JSON sender-module specific data.
 * 
 * [bin]: binary conntent maybe attached to the command.
 */
#define TYPE_AndruavMessage_Sync_EventFire              1061
#define TYPE_AndruavMessage_SearchTargetList            1062

//! NOT USED YET
#define TYPE_AndruavMessage_Prepherials                 1070
/**
 * @brief: sends information about UDP Proxy of the unit.
 * a:  string - udp_ip_other
 * p:  int - udp_port_other
 * o:  int - optimization_level
 * en: bool - enabled
 * z: bool - paused
 */
#define TYPE_AndruavMessage_UDPProxy_Info               1071
/**
 * @brief used to set unit name and description.
 * This message is mainly sent from web and received by communication module.
 * It is used to change unit name and description.
*/
#define TYPE_AndruavMessage_Unit_Name                   1072
/**
 * @brief used to ping a unit name.
 * This message works in two ways:
 * * 1- send a ping to a unit to tell it that I am alive via p2p.
 * * 2- This is similar to send RemoteExecute (TYPE_AndruavMessage_ID)
 *      But in this case target unit does not need to reply with TYPE_AndruavMessage_ID
 *      It can reply with same TYPE_AndruavMessage_Ping_Unit
 *  Note that 1 & 2 can be done in a single message.
 * 
 * params:
 *      [a]: sender_party_id : drone_engage party id. case: #1
 *      [k]: 1 - request ack.                         case: #2
 */
#define TYPE_AndruavMessage_Ping_Unit                   1073

/**
 * @brief used to upload DroneEngage Mission File.
 * 
 * params:
 *      [a]: p_textMission,
 *      [e]: p_eraseFirst
 */
#define TYPE_AndruavMessage_Upload_DE_Mission           1075


#define TYPE_AndruavMessage_AI_Recognition_ACTION               1076
#define TYPE_AndruavMessage_AI_Recognition_STATUS               1077
#define TYPE_AndruavMessage_AI_Recognition_TargetLocation       1078

#define TYPE_AndruavMessage_Viewlink_ACTION                    1079
#define TYPE_AndruavMessage_Viewlink_STATUS                    1080
#define TYPE_AndruavMessage_DEPilot_Control                    1081



//Binary Starts with 2000

//deprecated telemetry technology
#define TYPE_AndruavMessage_LightTelemetry              2022

/**********************************************************************
                        New Andruav Messages 2019
**********************************************************************/
#define TYPE_AndruavMessage_ServoChannel                       6001

#define TYPE_AndruavMessage_MAVLINK                            6502
#define TYPE_AndruavMessage_SWARM_MAVLINK                      6503

/**
 * Used by other modules to exchange mavlink information
 * between each other.
 * This allows custom implementation for sharing mavlink info 
 * between mavlink module and other modules.
*/
#define TYPE_AndruavMessage_INTERNAL_MAVLINK                   6504


#define TYPE_AndruavMessage_P2P_ACTION                         6505
#define TYPE_AndruavMessage_P2P_STATUS                         6506

#define TYPE_AndruavMessage_P2P_InRange_BSSID                  6507
#define TYPE_AndruavMessage_P2P_InRange_Node                   6508


/**
 * @brief used to set communication channels on/off
 * current fields are:
 * [p2p]: for turning p2p on/off or leave as is.
 * [ws]: for turning communication server websocket on/off or leave as is.
 * [w2]: for turning communication server LOCAL websocket on/off or leave as is. 
 * [p2]: ip of [w2] which means reconnect to this ip Local Communication Socket.
 */
#define TYPE_AndruavMessage_Communication_Line_Set             6509

#define TYPE_AndruavMessage_Communication_Line_Status          6510


#define TYPE_AndruavMessage_SOUND_TEXT_TO_SPEECH               6511
#define TYPE_AndruavMessage_SOUND_PLAY_FILE                    6512


/**
 * @brief MODULE_ACTION is a generic module message. 
 * In SDR it is used to configure the module.
 * current fields are:
 * 
 * CMD#1
 * [a]: SDR_ACTION_SDR_INFO                                 6
 * [fc]: center frequency
 * [g]: gain
 * [r]: sample rate
 * [m]: demodulation mode -NOT IMPLEMENTED-
 * [i]: driver index, based on TYPE_AndruavMessage_SDR_INFO
 * [t]: rate of reading signals. - 0 means once
 * [r]: display bars... i.e. # of merged output readings.
 * [l]: trigger level... signal level after which a trigger event is sent.
 * 
 * **********************************************************************
 * CMD#2
 * [a]:  SDR_ACTION_LIST_SDR_DEVICES                        2
 * [dr]: drivers list
 * 
 * CMD#3
 * [a]: SDR_ACTION_TRIGGER                                  7
 */
#define TYPE_AndruavMessage_SDR_ACTION                         6514
#define TYPE_AndruavMessage_SDR_REMOTE_EXECUTE                 6515
#define TYPE_AndruavMessage_SDR_SPECTRUM                       6516

// GPIO Parameters
#define GPIO_ACTION_PORT_CONFIG                                0
#define GPIO_ACTION_INFO                                       1
#define GPIO_ACTION_PORT_WRITE                                 2

#define TYPE_AndruavMessage_P2P_INFO                           6517


#define TYPE_AndruavMessage_Mission_Item_Sequence              6518


#define TYPE_AndruavMessage_GPIO_ACTION                        6519
#define TYPE_AndruavMessage_GPIO_STATUS                        6520
#define TYPE_AndruavMessage_GPIO_REMOTE_EXECUTE                6521

/**
 * @brief Set IP/Port of Local Communication Server.
 * current fields are:
 * [u]: url/ip
 * [p]: port
 */
#define TYPE_AndruavMessage_LocalServer_ACTION                 6522
#define TYPE_AndruavMessage_LocalServer_STATUS                 6523
#define TYPE_AndruavMessage_LocalServer_REMOTE_EXECUTE         6524

#define TYPE_AndruavMessage_CONFIG_ACTION                      6525
#define TYPE_AndruavMessage_CONFIG_STATUS                      6526


#define TYPE_AndruavMessage_MAVLINK_EVENTS                     6527

#define TYPE_AndruavMessage_IR_CAMERA_MI48_ACTION              6528
#define TYPE_AndruavMessage_IR_CAMERA_MI48_STATUS              6529
#define TYPE_AndruavMessage_SOUND_LIST                         6530
/**
 * @brief Remote Telnet/Terminal messages.
 * @details Allows a webclient to open a remote shell session on a unit,
 * send keystrokes, and receive terminal output. The de_telnet module
 * owns the pty lifecycle; de_comm routes these messages like any other
 * module-class message.
 *
 * TELNET_ACTION_OPEN    - open a new session. Reply with TELNET_STATUS.
 * TELNET_ACTION_CLOSE   - close a session by session_id.
 * TELNET_ACTION_LIST    - request list of active sessions.
 * TELNET_ACTION_RESIZE  - resize pty window (cols/rows).
 * TELNET_ACTION_DATA    - keystrokes/input from client (binary payload).
 *
 * JSON fields (in "ms" / ANDRUAV_PROTOCOL_MESSAGE_CMD):
 *   "a": action code (TELNET_ACTION_*)
 *   "i": session_id (string, assigned by module on OPEN)
 *   "d": text data (string) for DATA action when not using binary attach
 *   "c": columns (int) for RESIZE
 *   "r": rows    (int) for RESIZE
 *   "sh": shell  (string, optional) override shell binary for OPEN
 *   "st": status code (int) for TELNET_STATUS
 *   "e": error message (string) for TELNET_STATUS on failure
 *   "l": array of session info objects for LIST reply
 *
 * Binary path: TELNET_DATA may carry raw bytes as the binary attachment
 * after the JSON header (see CModule::sendBMSG). The "i" field in the
 * JSON header identifies the target session.
 */
#define TYPE_AndruavMessage_TELNET_ACTION                      6531
#define TYPE_AndruavMessage_TELNET_STATUS                      6532
#define TYPE_AndruavMessage_TELNET_DATA                        6533
#define TYPE_AndruavMessage_TELNET_REMOTE_EXECUTE              6534


/**
 * @brief Periodic self-reported module health/memory status (Layer 2 of the
 * DroneEngage Performance Monitor design - see servers/droneengage_performance_monitor
 * README). Sent by CFacade_Base::sendMemoryStatus(), called periodically from a
 * module's own main loop. Any C++ module linking de_common gets this "for free".
 *
 * fields:
 * [a]:  MODULE_HEALTH_ACTION_* (currently only MODULE_HEALTH_ACTION_STATUS)
 * [rs]: current resident memory (RSS) in MB
 * [pk]: peak resident-adjacent memory (VmPeak) in MB - a high/still-rising VmPeak
 *       with RSS tracking it indicates memory that is allocated but never released.
 * [sw]: swapped-out memory (VmSwap) in MB - non-zero/growing indicates memory
 *       pressure even before RSS itself looks alarming.
 * [th]: thread count - a leaking thread count is a distinct failure mode from a
 *       leaking heap and is cheap to include.
 * [sl]: RSS growth rate in MB/hour (linear regression over the rolling window)
 * [tr]: MODULE_HEALTH_TREND_* - UP/DOWN/STABLE, derived from [sl]
 * [hs]: MODULE_HEALTH_STATUS_* - OK/WARNING/CRITICAL, derived from [rs]/[sl]
 *       against configurable thresholds (CFacade_Base::configureMemoryStatus())
 * [up]: seconds since this module's health monitor started sampling - lets the
 *       receiver discount slope/trend during the warm-up period after a (re)start.
 *
 * Module identity (module_id/module_key/party_id) is not repeated here - it is
 * already carried by the surrounding sendJMSG() envelope.
 */
#define TYPE_AndruavMessage_MODULE_HEALTH_STATUS                6535


#define TYPE_AndruavMessage_DUMMY                              9999


// System Messages
#define TYPE_AndruavSystem_LoadTasks	              9001
#define TYPE_AndruavSystem_SaveTasks	              9002
#define TYPE_AndruavSystem_DeleteTasks	              9003
#define TYPE_AndruavSystem_DisableTasks	              9004
#define TYPE_AndruavSystem_Ping                         9005
#define TYPE_AndruavSystem_LogoutCommServer             9006
#define TYPE_AndruavSystem_ConnectedCommServer          9007
#define TYPE_AndruavSystem_UdpProxy                     9008
#define TYPE_AndruavSystem_LocalServer                  9009

// Inter Module Commands
#define TYPE_AndruavModule_ID                           9100
#define TYPE_AndruavModule_RemoteExecute                9101
#define TYPE_AndruavModule_Location_Info                9102


// #define TYPE_AndruavMessage_Sonar_Info              13001
// #define TYPE_AndruavMessage_Sonar_Action            13002
// #define TYPE_AndruavMessage_Sonar_RemoteExecute     13003

// DEFINE YOUR MESSAGE NUMBER HERE
#define TYPE_AndruavMessage_USER_RANGE_START 80000
#define TYPE_AndruavMessage_USER_RANGE_END 90000

// Andruav Mission Types

#define TYPE_CMissionItem                                   0
#define TYPE_CMissionItem_WayPointStep                      16 // same as mavlink
#define TYPE_CMissionAction_TakeOff                         22 // same as mavlink
#define TYPE_CMissionAction_Landing                         21 // same as mavlink
#define TYPE_CMissionAction_RTL                             20 // same as mavlink
#define TYPE_CMissionAction_Circle                          18 // same as mavlink MAV_CMD_NAV_LOITER_TURNS
#define TYPE_CMissionAction_Guided_Enabled                  92 // same as mavlink
#define TYPE_CMissionAction_Spline                          6
#define TYPE_CMissionAction_ChangeSpeed                    178 // same as mavlink
#define TYPE_CMissionAction_ChangeAlt                      113 // same as mavlink   
#define TYPE_CMissionAction_CONTINUE_AND_CHANGE_ALT         30  // same as mavlink  
#define TYPE_CMissionAction_ChangeHeading                  115 // same as mavlink 
#define TYPE_CMissionAction_Delay                           93 // same as mavlink 
#define TYPE_CMissionAction_Delay_STATE_MACHINE            112 // same as mavlink
#define TYPE_CMissionAction_DummyMission                 99999



// P2P Parameters

#define P2P_ACTION_RESTART_TO_MAC                           0
#define P2P_ACTION_CONNECT_TO_MAC                           1
#define P2P_ACTION_CANDICATE_MAC                            2
#define P2P_ACTION_SCAN_NETWORK                             3
/**
 * @brief this is different from P2P_ACTION_CONNECT_TO_MAC 
 * in that it does not require direct access 
 * or specifies who is parent to whom.
 */
#define P2P_ACTION_ACCESS_TO_MAC                            4
#define P2P_ACTION_SEND_STATUS                              5

#define P2P_STATUS_CONNECTED_TO_MAC                         0
#define P2P_STATUS_DISCONNECTED_FROM_MAC                    1


// Remote Control Sub Actions
#define RC_SUB_ACTION_RELEASED                              0
#define RC_SUB_ACTION_CENTER_CHANNELS                       1
#define RC_SUB_ACTION_FREEZE_CHANNELS                       2
#define RC_SUB_ACTION_JOYSTICK_CHANNELS                     4
#define RC_SUB_ACTION_JOYSTICK_CHANNELS_GUIDED              8

// Remote Execute Commands
#define RemoteCommand_GET_WAY_POINTS             500 // get from andruav not FCB but you can still read from fcb and refresh all   
#define RemoteCommand_RELOAD_WAY_POINTS_FROM_FCB 501
#define RemoteCommand_CLEAR_WAY_POINTS  502
#define RemoteCommand_CLEAR_WAY_POINTS  502
#define RemoteCommand_CLEAR_FENCE_DATA 	         503 // andruav fence
#define RemoteCommand_SET_START_MISSION_ITEM     504
#define RemoteCommand_TELEMETRYCTRL              108 // Telemetry streaming
#define RemoteCommand_STREAMVIDEO                110


// Drone Report
#define Drone_Report_NAV_ItemReached            1

// Error Info Types

#define NOTIFICATION_TYPE_REGISTRATION          22
#define NOTIFICATION_TYPE_TELEMETRY             33
#define NOTIFICATION_TYPE_PROTOCOL              44
#define NOTIFICATION_TYPE_LO7ETTA7AKOM          77
#define NOTIFICATION_TYPE_GEO_FENCE             88

// Error Numbers
#define ERROR_TYPE_LO7ETTA7AKOM                 5
#define ERROR_3DR                               7
#define ERROR_GPS                               10
#define ERROR_POWER                             11
#define ERROR_TYPE_ERROR_MODULE                 13
#define ERROR_TYPE_ERROR_P2P                    23
#define ERROR_TYPE_ERROR_SDR                    24
#define ERROR_GEO_FENCE_ERROR                   100

// 0	MAV_SEVERITY_EMERGENCY	System is unusable. This is a "panic" condition.
#define NOTIFICATION_TYPE_EMERGENCY             0
// 1	MAV_SEVERITY_ALERT	Action should be taken immediately. Indicates error in non-critical systems.
#define NOTIFICATION_TYPE_ALERT                 1
// 2	MAV_SEVERITY_CRITICAL	Action must be taken immediately. Indicates failure in a primary system.
#define NOTIFICATION_TYPE_CRITICAL              2
// 3	MAV_SEVERITY_ERROR	Indicates an error in secondary/redundant systems.
#define NOTIFICATION_TYPE_ERROR                 3
// 4	MAV_SEVERITY_WARNING	Indicates about a possible future error if this is not resolved within a given timeframe. Example would be a low battery warning.
#define NOTIFICATION_TYPE_WARNING               4
// 5	MAV_SEVERITY_NOTICE	An unusual event has occurred, though not an error condition. This should be investigated for the root cause.
#define NOTIFICATION_TYPE_NOTICE                5
// 6	MAV_SEVERITY_INFO	Normal operational messages. Useful for logging. No action is required for these messages.
#define NOTIFICATION_TYPE_INFO                  6
// 7	MAV_SEVERITY_DEBUG	Useful non-operational messages that can assist in debugging. These should not occur during normal operation.
#define NOTIFICATION_TYPE_DEBUG                 7




// Telemetry Request Remote Execute
#define CONST_TELEMETRY_REQUEST_START		1
#define CONST_TELEMETRY_REQUEST_END			2
#define CONST_TELEMETRY_REQUEST_RESUME		3
#define CONST_TELEMETRY_ADJUST_RATE		    4
#define CONST_TELEMETRY_REQUEST_PAUSE       5



#define GPS_MODE_AUTO                           0
// .a.k.a mobile... i.e. gps info used bu de comm is not from the board
#define GPS_MODE_EXTERNAL                       1
#define GPS_MODE_FCB                            2




#define WAYPOINT_NO_CHUNK                       0
#define WAYPOINT_CHUNK                          1
#define WAYPOINT_LAST_CHUNK                     999


#define FORMATION_NO_SWARM                      0


// GCS Permissions
#define PERMISSION_ALLOW_GCS                0x00000001
#define PERMISSION_ALLOW_UNIT               0x00000010
#define PERMISSION_ALLOW_GCS_FULL_CONTROL   0x00000f00
#define PERMISSION_ALLOW_GCS_WP_CONTROL     0x00000100
#define PERMISSION_ALLOW_GCS_MODES_CONTROL  0x00000200
#define PERMISSION_ALLOW_GCS_MODES_SERVOS   0x00000400
#define PERMISSION_ALLOW_GCS_VIDEO          0x0000f000



#define SPECIAL_NAME_ANY                "_any_"
#define SPECIAL_NAME_ALL_RECEIVERS      "_generic_"
#define SPECIAL_NAME_VEHICLE_RECEIVERS  "_drone_"
#define SPECIAL_NAME_GCS_RECEIVERS      "_gcs_"

// TYPE_AndruavMessage_TrackingTarget_ACTION
#define TargetTracking_ACTION_TRACKING_POINT    0
#define TargetTracking_ACTION_TRACKING_REGION   1
#define TargetTracking_ACTION_TRACKING_STOP     2
#define TargetTracking_ACTION_TRACKING_PAUSE    3
#define TargetTracking_ACTION_TRACKING_ENABLE   4 

// TYPE_AndruavMessage_TrackingTarget_STATUS
#define TargetTracking_STATUS_TRACKING_LOST     0
#define TargetTracking_STATUS_TRACKING_DETECTED 1
#define TargetTracking_STATUS_TRACKING_ENABLED  2
#define TargetTracking_STATUS_TRACKING_STOPPED  3


// TYPE_AndruavMessage_TrackingTarget_ACTION
#define TargetTracking_ACTION_AI_Recognition_POINT          0
#define TargetTracking_ACTION_AI_Recognition_SEARCH         1
#define TargetTracking_ACTION_AI_Recognition_DISABLE        2
#define TargetTracking_ACTION_AI_Recognition_ENABLE         3
#define TargetTracking_ACTION_AI_Recognition_CLASS_LIST     4



// TYPE_AndruavMessage_AI_Recognition_STATUS
#define TargetTracking_STATUS_AI_Recognition_LOST           0
#define TargetTracking_STATUS_AI_Recognition_DETECTED       1
#define TargetTracking_STATUS_AI_Recognition_ENABLED        2
#define TargetTracking_STATUS_AI_Recognition_DISABLED       3
#define TargetTracking_STATUS_AI_Recognition_CLASS_LIST     4


// TYPE_AndruavMessage_CONFIG_ACTION
#define CONFIG_ACTION_Restart                               0
#define CONFIG_ACTION_APPLY_CONFIG                          1
#define CONFIG_REQUEST_FETCH_CONFIG_TEMPLATE                2
#define CONFIG_REQUEST_FETCH_CONFIG                         3
#define CONFIG_ACTION_SHUT_DOWN_HW                          4
#define CONFIG_ACTION_RESTART_HW                            5

#define CONFIG_STATUS_FETCH_CONFIG_TEMPLATE                 0
#define CONFIG_STATUS_FETCH_CONFIG                          1


// TYPE_AndruavMessage_TELNET_ACTION
#define TELNET_ACTION_OPEN                                  0
#define TELNET_ACTION_CLOSE                                 1
#define TELNET_ACTION_LIST                                  2
#define TELNET_ACTION_RESIZE                                3
#define TELNET_ACTION_DATA                                  4   // input from client

// TYPE_AndruavMessage_TELNET_STATUS
#define TELNET_STATUS_OPENED                                0   // session opened ok
#define TELNET_STATUS_CLOSED                                1   // session closed
#define TELNET_STATUS_DATA                                  2   // output data from pty
#define TELNET_STATUS_LIST                                  3   // list of sessions
#define TELNET_STATUS_ERROR                                 4   // error (see "e" field)
#define TELNET_STATUS_RESIZED                               5   // resize ack


// TYPE_AndruavMessage_MODULE_HEALTH_STATUS
#define MODULE_HEALTH_ACTION_STATUS                         0

// MODULE_HEALTH_STATUS_* : field [hs]
#define MODULE_HEALTH_STATUS_OK                             0
#define MODULE_HEALTH_STATUS_WARNING                        1
#define MODULE_HEALTH_STATUS_CRITICAL                        2

// MODULE_HEALTH_TREND_* : field [tr]
#define MODULE_HEALTH_TREND_STABLE                          0
#define MODULE_HEALTH_TREND_UP                              1
#define MODULE_HEALTH_TREND_DOWN                            2