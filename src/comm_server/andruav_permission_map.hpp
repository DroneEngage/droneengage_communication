#ifndef ANDRUAV_PERMISSION_MAP_H_
#define ANDRUAV_PERMISSION_MAP_H_

#include <cstdint>
#include <string>

#include "../helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;

/**
 * Returns the required permission bit for a message type.
 * Returns 0 if the message type requires no permission (telemetry/status/view).
 */
uint32_t fn_getRequiredPermission(int message_type);

/**
 * Centralized permission gate.  Called by the comm server BEFORE the
 * parser and BEFORE processIncommingServerMessage so that denied
 * messages are dropped entirely and never forwarded to modules.
 *
 * For TYPE_AndruavMessage_RemoteExecute the check is performed on the
 * sub-command stored in the "C" field of the message body, since
 * RemoteExecute is a wrapper that delegates to the real command.
 *
 * View-mode gate: if the sender's permission has the view-mode bit
 * (PERMISSION_ALLOW_VIEW_MODE) set, ONLY TYPE_AndruavMessage_ID is
 * allowed — all other message types are rejected, including unmapped
 * types that would otherwise pass the per-bit map (required == 0).
 * This closes the gap where unmapped command types bypass the per-bit
 * enforcement and reach the parser/broker.
 *
 * @param jsonMessage   The full incoming JSON envelope (contains "p",
 *                      "sd", and "cm" fields).
 * @param command_type  The outer message type
 *                      (TYPE_AndruavMessage_*).
 * @return true if the message is allowed (permission granted or
 *         is_system), false if it should be rejected.
 */
bool fn_checkMessagePermission(const Json_de &jsonMessage, int command_type);

#endif // ANDRUAV_PERMISSION_MAP_H_
