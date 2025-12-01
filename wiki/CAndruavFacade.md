`CAndruavFacade` is a singleton class providing high-level APIs for communication with the Andruav Communication Server.  
It serves as the central interface for sending standardized messages and commands from internal modules to remote parties or the server.

---

### Definition

```cpp
21:71:/mnt/8a619ce7-cd3f-4520-af65-7991f16410f7/public_versions/drone_engage/drone_engage_communication_pro/src/comm_server/andruav_facade.hpp
class CAndruavFacade
{
    public:
        // Implements the Meyers Singleton pattern
        static CAndruavFacade& getInstance()
        {
            static CAndruavFacade instance;
            return instance;
        }

        // Prevent copying and assignment
        CAndruavFacade(CAndruavFacade const&) = delete;
        void operator=(CAndruavFacade const&) = delete;

    private:
        CAndruavFacade(); // Private constructor
        ~CAndruavFacade(){}; // Public destructor

    public:
        void API_sendID (const std::string& target_party_id) const;
        void API_requestID (const std::string& target_party_id) const;
        void API_sendCameraList (const bool reply, const std::string& target_party_id) const;
        void API_sendErrorMessage (
            const std::string& target_party_id,
            const int& error_number,
            const int& info_type,
            const int& notification_type,
            const std::string& description
        ) const;
        
        void API_loadTasksByScope (const ENUM_TASK_SCOPE scope, const int task_type) const;
        void API_loadTasksByScopeGlobal (const int task_type) const;
        void API_loadTasksByScopeAccount (const int task_type) const;
        void API_loadTasksByScopeGroup (const int task_type) const;
        void API_loadTasksByScopePartyID (const int task_type) const;

        void API_loadTask (
            const int larger_than_SID,
            const std::string& account_id,
            const std::string& party_sid,
            const std::string& group_name,
            const std::string& sender,
            /* ... more params ... */
        );

        void API_sendPrepherals (const std::string& target_party_id) const;
        void API_sendCommunicationLineStatus(const std::string& target_party_id, const bool on_off) const;
        void API_sendConfigTemplate(
            const std::string& target_party_id,
            const std::string& module_key,
            const Json_de& json_file_content_json,
            const bool reply
        );

    protected:
        void API_sendCMD (
            const std::string& target_party_id,
            const int command_type,
            const Json_de& msg
        ) const;

        const std::string API_sendCMDDummy (
            const std::string& target_party_id,
            const int command_type,
            const Json_de& msg
        ) const;
};
```

- **Type**: Singleton class (`CAndruavFacade`)
- **Namespace**: `de::andruav_servers`
- **Purpose**: Centralized message dispatching to the Andruav Communication Server
- **Access Pattern**: Thread-safe singleton via `getInstance()`
- **Key Methods**:
  - `API_sendID`: Acknowledge identity to a remote party
  - `API_requestID`: Request identification from a remote party
  - `API_sendErrorMessage`: Send error alerts to server or unit
  - `API_loadTasksByScope*`: Load mission or configuration tasks by scope (global, account, group, etc.)
  - `API_sendConfigTemplate`: Send JSON configuration templates to a module
  - `API_sendCommunicationLineStatus`: Notify server about connection status
- **Side Effects**: All methods generate outbound network messages via an underlying communication layer
- **Thread Safety**: Static `getInstance()` ensures one instance; thread safety relies on `static` initialization rules in C++11+

> 🔒 The class enforces non-copyability and private construction to ensure singleton integrity.

---

### Example Usages

In practice, `CAndruavFacade` is used across multiple subsystems to send standardized messages without directly handling raw protocol details.

For example, when a module fails license verification, an error message is sent using the facade:

```cpp
518:519:/home/mhefny/TDisk/public_versions/drone_engage/drone_engage_communication_pro/src/de_broker/de_modules_manager.cpp
module_item->licence_status = ENUM_LICENCE::LICENSE_VERIFIED_BAD;
andruav_servers::CAndruavFacade::getInstance().API_sendErrorMessage(
    std::string(), 
    0, 
    ERROR_TYPE_ERROR_MODULE, 
    NOTIFICATION_TYPE_ALERT, 
    std::string("Module " + module_item->module_id + " invalid license")
);
```

Another example occurs during system initialization upon successful server connection:

```cpp
384:/home/mhefny/TDisk/public_versions/drone_engage/drone_engage_communication_pro/src/comm_server/andruav_comm_server.cpp
CAndruavFacade::getInstance().API_requestID(std::string());
```

This triggers a request for the local unit’s identity from the server.

#### Usage Summary

- **Main Callers**:
  - `de_modules_manager.cpp`: Handles module lifecycle, sends errors and ID updates
  - `andruav_parser.cpp`: Processes incoming messages, responds with camera lists, config templates, or error reports
  - `andruav_comm_server.cpp`: Manages server connection state, sends line status and requests ID
- **Frequency**: Heavily used — referenced in at least 9 files, with dozens of calls
- **Integration Level**: Core communication abstraction used by both broker and server modules

---

### Notes

- `CAndruavFacade` uses the **Meyers Singleton** pattern (line 26–30), which is thread-safe in C++11 due to static local variable initialization guarantees.
- Despite its name suggesting a pure facade, it likely wraps a lower-level messaging system (e.g., WebSocket or TCP client) that isn't exposed here.
- The method `API_sendCMDDummy` returns a `const std::string`, suggesting it may simulate or log commands rather than transmit them — useful for testing or dry runs.

---

### See Also

- `ENUM_TASK_SCOPE`: Defines scoping levels (`SCOPE_GLOBAL`, `SCOPE_ACCOUNT`, etc.) used in task-loading APIs like `API_loadTasksByScope`. Determines which set of tasks to retrieve from the server.
- `Json_de`: A JSON library wrapper (likely a typedef for `nlohmann::json`) used to serialize message payloads in methods such as `API_sendConfigTemplate`.
- `CAndruavUnitMe`: Represents the local drone unit; often used alongside `CAndruavFacade` when sending unit-specific status or errors.
- `API_sendCMD`: The underlying generic command sender used internally by higher-level APIs to format and dispatch messages.
