

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
#define SPECIAL_NAME_SYS_NAME                   "_SYS_"


// Andruav Protocol Fields
#define ANDRUAV_PROTOCOL_GROUP_ID               "gr"
#define ANDRUAV_PROTOCOL_SENDER                 "sd"
#define ANDRUAV_PROTOCOL_TARGET_ID              "tg"
#define ANDRUAV_PROTOCOL_MESSAGE_TYPE           "mt"
#define ANDRUAV_PROTOCOL_MESSAGE_CMD            "ms"
#define ANDRUAV_PROTOCOL_MESSAGE_PERMISSION     "p"
#define INTERMODULE_ROUTING_TYPE                "ty"
#define INTERMODULE_MODULE_KEY                  "GU"

// System Messages
#define TYPE_AndruavSystem_LoadTasks		    9001
#define TYPE_AndruavSystem_SaveTasks		    9002
#define TYPE_AndruavSystem_DeleteTasks	        9003
#define TYPE_AndruavSystem_DisableTasks	        9004
#define TYPE_AndruavSystem_Ping                 9005
#define TYPE_AndruavSystem_LogoutCommServer     9006
#define TYPE_AndruavSystem_ConnectedCommServer  9007
#define TYPE_AndruavSystem_UdpProxy             9008

// Inter Module Commands
#define TYPE_AndruavModule_ID                   9100
#define TYPE_AndruavModule_RemoteExecute        9101
#define TYPE_AndruavModule_Location_Info        9102


// Andruav Messages
#define TYPE_AndruavMessage_GPS                     1002
#define TYPE_AndruavMessage_POWER                   1003
#define TYPE_AndruavMessage_ID 	                    1004
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
#define TYPE_AndruavMessage_RemoteExecute 		    1005
#define TYPE_AndruavMessage_IMG                     1006
#define TYPE_AndruavMessage_Error                   1008    
#define TYPE_AndruavMessage_FlightControl           1010
#define TYPE_AndruavMessage_CameraList 			    1012  //RX: {"tg":"GCS1","sd":"zxcv","ty":"c","gr":"1","cm":"i","mt":1012,"ms":"{\"E\":2,\"P\":0,\"I\":\"zxcv\"}"}
#define TYPE_AndruavMessage_DroneReport             1020
#define TYPE_AndruavMessage_Signaling               1021
#define TYPE_AndruavMessage_HomeLocation            1022
#define TYPE_AndruavMessage_GeoFence                1023
#define TYPE_AndruavMessage_ExternalGeoFence        1024
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
#define TYPE_AndruavMessage_TrackingTarget          1042
#define TYPE_AndruavMessage_TargetLoackedAt         1043
#define TYPE_AndruavMessage_TargetLost              1044
#define TYPE_AndruavMessage_UploadWayPoints         1046
#define TYPE_AndruavMessage_RemoteControlSettings	1047
#define TYPE_AndruavMessage_SET_HOME_LOCATION       1048
#define TYPE_AndruavMessage_CameraFlash		        1051
#define TYPE_AndruavMessage_RemoteControl2		    1052
#define TYPE_AndruavMessage_FollowHim_Request       1054
#define TYPE_AndruavMessage_FollowMe_Guided         1055
#define TYPE_AndruavMessage_MAKE_SWARM              1056
#define TYPE_AndruavMessage_UpdateSwarm             1058
#define TYPE_AndruavMessage_Prepherials             1070
#define TYPE_AndruavMessage_UDPProxy_Info           1071
/**
 * @brief used to set unit name and description.
 * This message is mainly sent from web and received by communication module.
 * It is used to change unit name and description.
*/
#define TYPE_AndruavMessage_Unit_Name               1072
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

#define TYPE_AndruavMessage_P2P_INFO                    1074

//Binary Starts with 2000

//deprecated telemetry technology
#define TYPE_AndruavMessage_LightTelemetry              2022

/**********************************************************************
                        New Andruav Messages 2019
**********************************************************************/
#define TYPE_AndruavMessage_ServoChannel                6001

#define TYPE_AndruavMessage_MAVLINK                     6502
#define TYPE_AndruavMessage_SWARM_MAVLINK               6503

/**
 * Used by other modules to exchange mavlink information
 * between each other.
 * This allows custom implementation for sharing mavlink info 
 * between mavlink module and other modules.
*/
#define TYPE_AndruavMessage_INTERNAL_MAVLINK            6504


#define TYPE_AndruavMessage_P2P_ACTION                  6505
#define TYPE_AndruavMessage_P2P_STATUS                  6506

#define TYPE_AndruavMessage_P2P_InRange_BSSID           6507
#define TYPE_AndruavMessage_P2P_InRange_Node            6508


/**
 * @brief used to set communication channels on/off
 * current fields are:
 * [p2p]: for turning p2p on/off or leave as is.
 * [ws]: for turning communication server websocket on/off or leave as is.
 * 
 */
#define TYPE_AndruavMessage_Communication_Line_Set          6509

#define TYPE_AndruavMessage_Communication_Line_Status       6510


#define TYPE_AndruavMessage_SOUND_TEXT_TO_SPEECH            6511
#define TYPE_AndruavMessage_SOUND_PLAY_FILE                 6512

#define TYPE_AndruavMessage_SDR_INFO                        6513
#define TYPE_AndruavMessage_SDR_ACTION                      6514
#define TYPE_AndruavMessage_SDR_STATUS                      6515


/**********************************************************************
                        EOF Andruav Messages 2019
**********************************************************************/



/**********************************************************************
                        EOF Binary Messages
**********************************************************************/


#define TYPE_AndruavMessage_DUMMY                       9999





#define TYPE_AndruavMessage_Sonar_Info              13001
#define TYPE_AndruavMessage_Sonar_Action            13002
#define TYPE_AndruavMessage_Sonar_RemoteExecute     13003


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
#define RemoteCommand_CLEAR_WAY_POINTS_FROM_FCB  502
#define RemoteCommand_CLEAR_WAY_POINTS_FROM_FCB  502
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

