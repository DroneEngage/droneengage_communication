#include "andruav_permission_map.hpp"
#include "../messages.hpp"
#include "../helpers/helpers.hpp"
#include <unordered_map>

/**
 * Centralized message-type → permission-bit mapping.
 *
 * Messages NOT in this map require no permission (telemetry/status/view).
 * The parser calls fn_getRequiredPermission() and checks the sender's
 * permission bitmask against the result. Comm-server-originated messages
 * (is_system) always bypass the check.
 */
static const std::unordered_map<int, uint32_t> s_permissionMap = {
    // ── Flight Control / Waypoints ──────────────────────────────────
    {TYPE_AndruavMessage_WayPoints,               PERMISSION_ALLOW_GCS_WP_CONTROL},
    {TYPE_AndruavMessage_UploadWayPoints,          PERMISSION_ALLOW_GCS_WP_CONTROL},
    {TYPE_AndruavMessage_Upload_DE_Mission,        PERMISSION_ALLOW_GCS_WP_CONTROL},
    {TYPE_AndruavMessage_GuidedPoint,              PERMISSION_ALLOW_GCS_WP_CONTROL},
    {TYPE_AndruavMessage_CirclePoint,              PERMISSION_ALLOW_GCS_WP_CONTROL},
    {TYPE_AndruavMessage_ChangeAltitude,           PERMISSION_ALLOW_GCS_WP_CONTROL},
    {TYPE_AndruavMessage_Land,                     PERMISSION_ALLOW_GCS_WP_CONTROL},
    {TYPE_AndruavMessage_Arm,                       PERMISSION_ALLOW_GCS_WP_CONTROL},
    {TYPE_AndruavMessage_ChangeSpeed,              PERMISSION_ALLOW_GCS_WP_CONTROL},
    {TYPE_AndruavMessage_SET_HOME_LOCATION,        PERMISSION_ALLOW_GCS_WP_CONTROL},
    {TYPE_AndruavMessage_HomeLocation,             PERMISSION_ALLOW_GCS_WP_CONTROL},
    {TYPE_AndruavMessage_DistinationLocation,      PERMISSION_ALLOW_GCS_WP_CONTROL},

    // ── Modes / RC ──────────────────────────────────────────────────
    {TYPE_AndruavMessage_FlightControl,           PERMISSION_ALLOW_GCS_MODES_CONTROL},
    {TYPE_AndruavMessage_RemoteControl2,           PERMISSION_ALLOW_GCS_MODES_CONTROL},
    {TYPE_AndruavMessage_RemoteControlSettings,    PERMISSION_ALLOW_GCS_MODES_CONTROL},
    {TYPE_AndruavMessage_DoYAW,                    PERMISSION_ALLOW_GCS_MODES_CONTROL},
    {TYPE_AndruavMessage_DEPilot_Control,          PERMISSION_ALLOW_GCS_MODES_CONTROL},

    // ── Servos ──────────────────────────────────────────────────────
    {TYPE_AndruavMessage_ServoChannel,             PERMISSION_ALLOW_GCS_MODES_SERVOS},

    // ── Video / Camera ──────────────────────────────────────────────
    {TYPE_AndruavMessage_Ctrl_Cameras,            PERMISSION_ALLOW_GCS_VIDEO},
    {TYPE_AndruavMessage_CameraZoom,              PERMISSION_ALLOW_GCS_VIDEO},
    {TYPE_AndruavMessage_CameraSwitch,            PERMISSION_ALLOW_GCS_VIDEO},
    {TYPE_AndruavMessage_CameraFlash,             PERMISSION_ALLOW_GCS_VIDEO},
    {TYPE_AndruavMessage_CameraList,              PERMISSION_ALLOW_GCS_VIDEO},
    {TYPE_AndruavMessage_Viewlink_ACTION,         PERMISSION_ALLOW_GCS_VIDEO},

    // ── Swarm ───────────────────────────────────────────────────────
    {TYPE_AndruavMessage_FollowHim_Request,        PERMISSION_ALLOW_SWARM},
    {TYPE_AndruavMessage_FollowMe_Guided,          PERMISSION_ALLOW_SWARM},
    {TYPE_AndruavMessage_Make_Swarm,               PERMISSION_ALLOW_SWARM},
    {TYPE_AndruavMessage_UpdateSwarm,              PERMISSION_ALLOW_SWARM},

    // ── Tracking / AI ───────────────────────────────────────────────
    {TYPE_AndruavMessage_TrackingTarget_ACTION,   PERMISSION_ALLOW_TRACKING},
    {TYPE_AndruavMessage_AI_Recognition_ACTION,    PERMISSION_ALLOW_TRACKING},

    // ── Geofence ────────────────────────────────────────────────────
    {TYPE_AndruavMessage_GeoFence,                 PERMISSION_ALLOW_GEOFENCE},
    {TYPE_AndruavMessage_ExternalGeoFence,         PERMISSION_ALLOW_GEOFENCE},

    // ── System / Config ─────────────────────────────────────────────
    {TYPE_AndruavMessage_CONFIG_ACTION,           PERMISSION_ALLOW_GCS_FULL_CONTROL},
    {TYPE_AndruavMessage_Unit_Name,                PERMISSION_ALLOW_CONFIG},
    {TYPE_AndruavMessage_LocalServer_ACTION,      PERMISSION_ALLOW_CONFIG},

    // ── P2P / Comm ──────────────────────────────────────────────────
    {TYPE_AndruavMessage_P2P_ACTION,              PERMISSION_ALLOW_P2P},
    {TYPE_AndruavMessage_Communication_Line_Set,   PERMISSION_ALLOW_P2P},

    // ── Sound ────────────────────────────────────────────────────────
    {TYPE_AndruavMessage_SOUND_TEXT_TO_SPEECH,     PERMISSION_ALLOW_SOUND},
    {TYPE_AndruavMessage_SOUND_PLAY_FILE,         PERMISSION_ALLOW_SOUND},

    // ── SDR ─────────────────────────────────────────────────────────
    {TYPE_AndruavMessage_SDR_ACTION,              PERMISSION_ALLOW_SDR},
    {TYPE_AndruavMessage_SDR_REMOTE_EXECUTE,      PERMISSION_ALLOW_SDR},

    // ── GPIO ─────────────────────────────────────────────────────────
    {TYPE_AndruavMessage_GPIO_ACTION,             PERMISSION_ALLOW_GPIO},
    {TYPE_AndruavMessage_GPIO_REMOTE_EXECUTE,     PERMISSION_ALLOW_GPIO},

    // ── Telnet ───────────────────────────────────────────────────────
    {TYPE_AndruavMessage_TELNET_ACTION,           PERMISSION_ALLOW_TELNET},
    {TYPE_AndruavMessage_TELNET_REMOTE_EXECUTE,   PERMISSION_ALLOW_TELNET},

    // ── RemoteExecute sub-commands (parseRemoteExecuteCommand) ─────
    // These use the RemoteCommand_* IDs, not TYPE_AndruavMessage_* IDs.
    // RemoteExecute (1005) is a wrapper: the real command is in the "C"
    // sub-field.  The comm server extracts "C" and checks it here.
    {RemoteCommand_GET_WAY_POINTS,              PERMISSION_ALLOW_GCS_WP_CONTROL},
    {RemoteCommand_RELOAD_WAY_POINTS_FROM_FCB,  PERMISSION_ALLOW_GCS_WP_CONTROL},
    {RemoteCommand_CLEAR_WAY_POINTS,            PERMISSION_ALLOW_GCS_WP_CONTROL},
    {RemoteCommand_CLEAR_FENCE_DATA,            PERMISSION_ALLOW_GEOFENCE},
    {RemoteCommand_SET_START_MISSION_ITEM,      PERMISSION_ALLOW_GCS_WP_CONTROL},
    {RemoteCommand_STREAMVIDEO,                 PERMISSION_ALLOW_GCS_VIDEO},
};

uint32_t fn_getRequiredPermission(int message_type)
{
    auto it = s_permissionMap.find(message_type);
    if (it != s_permissionMap.end())
    {
        return it->second;
    }
    return 0; // no permission required (telemetry/status/view)
}

bool fn_checkMessagePermission(const Json_de &jsonMessage, int command_type)
{
    // Extract the sender's permission bitmask from the "p" field.
    uint32_t permission = 0;
    if (validateField(jsonMessage, ANDRUAV_PROTOCOL_MESSAGE_PERMISSION,
                      Json_de::value_t::number_unsigned))
    {
        permission = jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_PERMISSION].get<int>();
    }

    // Comm-server-originated messages always bypass permission checks.
    if (validateField(jsonMessage, ANDRUAV_PROTOCOL_SENDER,
                      Json_de::value_t::string))
    {
        if (jsonMessage[ANDRUAV_PROTOCOL_SENDER].get<std::string>().compare(
                ANDRUAV_PROTOCOL_SENDER_COMM_SERVER) == 0)
        {
            return true; // is_system — always allowed
        }
    }

    // For RemoteExecute, the real command is in the "C" sub-field.
    // Check the sub-command's permission, not the outer wrapper type.
    int effective_command = command_type;
    if (command_type == TYPE_AndruavMessage_RemoteExecute)
    {
        const Json_de &msg_cmd =
            jsonMessage.contains(ANDRUAV_PROTOCOL_MESSAGE_CMD)
                ? jsonMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD]
                : Json_de();
        if (msg_cmd.contains("C") && msg_cmd["C"].is_number())
        {
            effective_command = msg_cmd["C"].get<int>();
        }
        else
        {
            // Malformed RemoteExecute — no sub-command to check.
            // Allow it through; the parser will handle the bad format.
            return true;
        }
    }

    uint32_t required = fn_getRequiredPermission(effective_command);

    // View-mode gate: a view-mode sender has bit 29 set AND no category/action
    // bits (bits 0-25).  This distinguishes a real view-mode account (0xa0000000)
    // from a full-control account that also has bit 29 set (0xffffffff).
    // A view-mode sender may only send ID messages — all other message types
    // are rejected, including unmapped types that would otherwise pass the
    // per-bit map (required == 0) and reach the parser/broker.
    if (((permission & PERMISSION_ALLOW_VIEW_MODE) == PERMISSION_ALLOW_VIEW_MODE)
        && ((permission & PERMISSION_CATEGORY_ACTION_MASK) == 0))
    {
        return (effective_command == TYPE_AndruavMessage_ID);
    }

    if (required == 0)
    {
        return true; // no permission needed (telemetry/status/view)
    }

    return (permission & required) == required;
}
