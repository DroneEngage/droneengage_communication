`CAndruavParser` is a singleton class responsible for parsing incoming JSON-based commands from remote parties in the DroneEngage communication system.  
It decodes and routes command messages to appropriate handlers based on message type, acting as a central dispatcher for protocol-level instructions.

---

### Definition

`CAndruavParser` is a thread-safe singleton class defined in the `de::andruav_servers` namespace. It provides static access to a single instance and exposes two primary parsing methods: `parseCommand` and `parseRemoteExecuteCommand`. The class is non-copyable and privately constructed, enforcing singleton usage.

```cpp
19:54:/mnt/8a619ce7-cd3f-4520-af65-7991f16410f7/public_versions/drone_engage/drone_engage_communication_pro/src/comm_server/andruav_parser.hpp
class CAndruavParser
{
    public:
        // Thread-safe singleton pattern (Meyers singleton)
        static CAndruavParser& getInstance()
        {
            static CAndruavParser instance;
            return instance;
        }

    public:
        // Prevent copying and assignment
        CAndruavParser(CAndruavParser const&) = delete;
        void operator=(CAndruavParser const&) = delete;

    private:
        CAndruavParser() {}

    public:
        ~CAndruavParser() {}

    public:
        // Parses general command messages
        void parseCommand(const std::string& sender_party_id, const int& command_type, const Json_de& jsonMessage);

        // Parses remote execution command messages
        void parseRemoteExecuteCommand(const std::string& sender_party_id, const Json_de& jsonMessage);

    private:
        // Reference to global unit registry
        CAndruavUnits& m_andruav_units = CAndruavUnits::getInstance();
};
```

- **Type**: Singleton class  
- **Namespace**: `de::andruav_servers`  
- **Methods**:
  - `getInstance()` – Returns the single global instance (thread-safe due to static local variable).
  - `parseCommand(...)` – Dispatches a command based on `command_type`.
  - `parseRemoteExecuteCommand(...)` – Handles execution commands, often involving remote control actions.
- **Dependencies**:
  - `Json_de` (aliased from `nlohmann::json`) for JSON parsing.
  - `CAndruavUnits` – Global registry of connected units, accessed via singleton.
- **Side effects**: Modifies internal state of units via `m_andruav_units`, may trigger command execution or status updates.

---

### Example Usages

`CAndruavParser` is invoked in `andruav_comm_server_local.cpp` to process incoming JSON messages from network clients. The message type determines whether `parseRemoteExecuteCommand` or the general `parseCommand` is called.

```cpp
314:323:/home/mhefny/TDisk/public_versions/drone_engage/drone_engage_communication_pro/src/comm_server/andruav_comm_server_local.cpp
case TYPE_AndruavMessage_RemoteExecute:
{
    CAndruavParser::getInstance().parseRemoteExecuteCommand(sender, jMsg);
}
break;

default:
{
    CAndruavParser::getInstance().parseCommand(sender, command_type, jMsg);
}
break;
```

This pattern appears in multiple locations within `andruav_comm_server_local.cpp`, indicating that `CAndruavParser` is a core component in the message dispatch pipeline. It is used consistently whenever a JSON message arrives from a remote sender and needs to be interpreted.

**Usage Summary**:
- Called exclusively through its singleton `getInstance()`.
- Used in `andruav_comm_server_local.cpp` to handle incoming messages.
- No direct instantiation; always accessed statically.
- Central to command routing — all high-level protocol messages pass through it.

---

### Notes

- The singleton implementation follows the Meyers singleton pattern (line 24–28), which is thread-safe in C++11 and later due to guaranteed static initialization concurrency safety.
- Despite being defined in a header, its methods are implemented in `andruav_parser.cpp`, indicating separation of interface and implementation.
- The class does not own any data directly — it operates through `m_andruav_units`, which suggests it functions primarily as a stateless processor/router.

---

### See Also

- `CAndruavUnits`: Global registry of connected devices; used by `CAndruavParser` to retrieve or create unit instances during command handling.
- `CAndruavUnit`: Represents an individual connected drone or client; commands parsed by `CAndruavParser` are typically applied to instances of this class.
- `Json_de`: Type alias for `nlohmann::json`, used throughout for JSON deserialization; essential for interpreting incoming messages.
- `andruav_comm_server_local.cpp`: Primary caller of `CAndruavParser`; contains the network message loop that feeds data into the parser.