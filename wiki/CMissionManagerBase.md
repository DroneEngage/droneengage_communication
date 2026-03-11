# SYNC EVENTS DRONEENGAGE MISSIONS

`CMissionManagerBase` is a singleton class responsible for managing mission items and implementing an event-driven architecture for triggering mission commands in the DroneEngage system.  
It parses DE mission plans, maintains event-to-command mappings, and executes commands when specific events occur during mission execution.

---

### Definition

`CMissionManagerBase` is a thread-safe singleton class defined in the `de::mission` namespace. It provides the core functionality for loading mission plans from JSON files, maintaining mappings between events and waiting commands, and executing those commands when events are triggered.

```cpp
/home/mhefny/TDisk/public_versions/drone_engage/drone_engage_communication_pro/src/de_general_mission_planner/mission_manager_base.hpp
class CMissionManagerBase
{
    public:
        // Thread-safe singleton pattern (Meyers singleton)
        static CMissionManagerBase& getInstance()
        {
            static CMissionManagerBase instance;
            return instance;
        }

    public:
        // Prevent copying and assignment
        CMissionManagerBase(CMissionManagerBase const&) = delete;
        void operator=(CMissionManagerBase const&) = delete;

    protected:
        CMissionManagerBase() {}

    public:
        ~CMissionManagerBase() {}

    public:
        // Extracts mission items from JSON plan
        virtual void extractPlanModule(const Json_de& plan);
        
        // Triggers commands waiting for specific events
        virtual void fireWaitingCommands(const std::string de_event_sid);
        
        // Executes commands attached to MAVLink mission steps
        virtual void getCommandsAttachedToMavlinkMission(const std::string mission_i);

    protected:
        // Internal storage for mission mappings
        std::map<std::string, std::vector<Json_de>> m_module_missions;
        std::map<std::string, std::vector<Json_de>> m_module_missions_by_de_events;
        std::string m_last_executed_mission_id = "";
};
```

- **Type**: Singleton class  
- **Namespace**: `de::mission`  
- **Key Methods**:
  - `extractPlanModule(...)` – Parses JSON mission plan and builds event/command mappings
  - `fireWaitingCommands(...)` – Executes all commands waiting for a specific event
  - `getCommandsAttachedToMavlinkMission(...)` – Executes commands linked to MAVLink mission steps
- **Dependencies**:
  - `Json_de` (aliased from `nlohmann::json`) for JSON parsing
  - `CAndruavParser` – For command parsing and execution
  - `CUavosModulesManager` – For module communication
  - `CMissionFile` – For file I/O operations

---

### Mission Items Logic

The mission manager supports two types of mission item triggering:

#### 1. Event-Driven Mission Items
Mission items can be configured to wait for specific events before execution:

```json
{
  "de_mission": {
    "modules": [
      {
        "c": [
          {
            "mt": 1005,
            "ms": "{\"C\":1,\"Act\":true}",
            "ty": "uv"
          }
        ],
        "ew": "EVENT_WAYPOINT_REACHED"
      }
    ]
  }
}
```

Key fields:
- `"ew"` (WAITING_EVENT): Event ID this command waits for
- `"c"`: Array of commands to execute when event occurs
- `"ty": "uv"`: Inter-module routing type

#### 2. MAVLink Step-Linked Mission Items
Commands can be linked to specific MAVLink mission sequence numbers:

```json
{
  "de_mission": {
    "modules": [
      {
        "c": [
          {
            "mt": 1041,
            "ms": "{\"action\":1}",
            "ty": "uv"
          }
        ],
        "ls": "3"
      }
    ]
  }
}
```

Key fields:
- `"ls"` (LINKED_TO_STEP): MAVLink mission sequence number
- Commands execute when the specified mission step becomes active

---

### Event Triggering System

The event triggering mechanism works through the following flow:

#### 1. Mission Loading Phase
```cpp
// From extractPlanModule() method
if (validateField(module_mission_item, WAITING_EVENT, Json_de::value_t::string))
{
    std::string de_event_id = module_mission_item[WAITING_EVENT].get<std::string>();
    addModuleMissionItemByEvent(de_event_id, module_mission_item_single_command);
}
```

#### 2. Event Firing Phase
```cpp
// From fireWaitingCommands() method
void CMissionManagerBase::fireWaitingCommands(const std::string de_event_sid)
{
    if (m_module_missions_by_de_events.find(de_event_sid) != m_module_missions_by_de_events.end()) 
    {
        std::vector<Json_de> cmds = m_module_missions_by_de_events[de_event_sid];
        
        for (const auto cmd : cmds)
        {
            const int command_type = cmd[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
            const std::string sender = de::CAndruavUnitMe::getInstance().getUnitInfo().party_id;
            
            // Route to appropriate handler
            switch (command_type)
            {
                case TYPE_AndruavMessage_RemoteExecute:
                    de::andruav_servers::CAndruavParser::getInstance().parseRemoteExecuteCommand(sender, cmd);
                    break;
                default:
                    de::andruav_servers::CAndruavParser::getInstance().parseCommand(sender, command_type, cmd);
                    break;
            }
            
            // Forward to module manager
            de::comm::CUavosModulesManager::getInstance().processIncommingServerMessage(
                sender, command_type, cmd_text.c_str(), cmd_text.length(), std::string());
        }
    }
}
```

---

### JSON Mission Format

The complete DE mission file structure:

```json
{
  "fileType": "de_plan",
  "version": 1,
  "unit": {
    "partyID": "1683012059",
    "unitName": "drone_cairo1",
    "vehichleType": 2,
    "home": {
      "lat": -35.36326217651367,
      "lng": 149.1652374267578,
      "alt": 0,
      "ft": 3
    }
  },
  "de_mission": {
    "mav_waypoints": [
      {
        "c": 16,
        "ft": 3,
        "mv": [0, 5, 0, 0, -35.3630553764387, 149.16505695778497, 30]
      }
    ],
    "modules": [
      {
        "c": [
          {
            "mt": 1005,
            "ms": "{\"C\":1,\"Act\":true}",
            "ty": "uv"
          }
        ],
        "ew": "CUSTOM_EVENT_ID"
      },
      {
        "c": [],
        "ls": "1"
      }
    ]
  },
  "de_geoFence": []
}
```

#### Key Constants
- `WAITING_EVENT` (`"ew"`): Event identifier field
- `FIRE_EVENT` (`"ef"`): Event firing field  
- `INTERMODULE_ROUTING_TYPE` (`"ty"`): Command routing type
- `LINKED_TO_STEP` (`"ls"`): MAVLink step linkage
- `CMD_TYPE_INTERMODULE` (`"uv"`): Inter-module command type

---

### Example Usages

#### Loading a Mission Plan
```cpp
// Load mission from JSON file
std::string mission_json = CMissionFile::getInstance().readMissionFile("de_plan.json");
Json_de plan = Json_de::parse(mission_json);
CMissionManagerBase::getInstance().extractPlanModule(plan);
```

#### Triggering Event-Based Commands
```cpp
// When waypoint is reached, fire waiting commands
CMissionManagerBase::getInstance().fireWaitingCommands("EVENT_WAYPOINT_REACHED");
```

#### MAVLink Mission Step Integration
```cpp
// When MAVLink mission step 3 becomes active
CMissionManagerBase::getInstance().getCommandsAttachedToMavlinkMission("3");
```

---

### Command Execution Flow

1. **Event Detection**: External system detects event (waypoint reached, sensor trigger, etc.)
2. **Event Firing**: `fireWaitingCommands()` called with event ID
3. **Command Lookup**: System retrieves all commands waiting for this event
4. **Command Parsing**: Each command parsed by `CAndruavParser`
5. **Module Routing**: Commands forwarded to `CUavosModulesManager`
6. **Execution**: Target modules execute the commands

---

### Integration Points

#### With CAndruavParser
- `parseRemoteExecuteCommand()`: Handles remote execution commands
- `parseCommand()`: Handles general command types
- Provides command type resolution and routing

#### With CUavosModulesManager  
- `processIncommingServerMessage()`: Routes commands to appropriate modules
- Handles inter-module communication
- Manages command delivery and execution

#### With CMissionFile
- `readMissionFile()`: Loads mission plans from disk
- `writeMissionFile()`: Saves mission plans to disk
- `deleteMissionFile()`: Cleans up mission files

---

### Notes

- **Singleton Pattern**: Uses Meyers singleton for thread-safe initialization
- **Event-Driven Architecture**: Decouples mission logic from event sources
- **Inter-Module Communication**: Commands marked with `"ty": "uv"` for inter-module routing
- **MAVLink Integration**: Supports both event-driven and step-based mission execution
- **State Management**: Tracks last executed mission to prevent duplicate execution

---

### See Also

- `CAndruavParser`: Command parsing and routing; used by mission manager for command execution
- `CUavosModulesManager`: Module communication system; handles command delivery to target modules
- `CMissionFile`: File I/O operations; manages mission plan persistence
- `Json_de`: JSON parsing library; essential for mission plan interpretation
- `messages.hpp`: Protocol constants and message type definitions
- `TYPE_AndruavMessage_RemoteExecute`: Remote execution command type (1005)
- `TYPE_AndruavMessage_Sync_EventFire`: Event firing message type (1061)
