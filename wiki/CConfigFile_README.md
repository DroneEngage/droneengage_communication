# CConfigFile::getInstance() - Configuration Management System

## Overview

`CConfigFile::getInstance()` implements a singleton pattern that provides centralized access to JSON-based configuration management in the DroneEngage Communication Module. It handles loading, parsing, monitoring, and updating of the main configuration file (`de_comm.config.module.json`).

## Class Architecture

### Singleton Pattern
```cpp
static CConfigFile& getInstance()
{
    static CConfigFile instance; // Guaranteed to be destroyed.
                                 // Instantiated on first use.
    return instance;
}
```

### Key Features
- **Thread-safe singleton** using Scott Meyers' implementation
- **File monitoring** with automatic reload on changes
- **JSON parsing** with C-style comment support
- **Nested key updates** (e.g., "follow_me.quad.PID_P_X")
- **Automatic backup** before saving changes
- **Runtime configuration updates**

## Configuration File Structure

The system reads from `de_comm.config.module.json` which contains:

```json
{
    // Module Identification
    "module_id": "MT_1",
    "unit_type": "control_unit",
    
    // Network Configuration
    "s2s_udp_listening_ip": "0.0.0.0",
    "s2s_udp_listening_port": "60000",
    "s2s_udp_packet_size": "8192",
    
    // Authentication Settings
    "auth_ip": "cloud.ardupilot.org",
    "auth_port": 19408,
    "auth_verify_ssl": false,
    "userName": "mhefny@andruav.com",
    "accessCode": "mhefny",
    
    // Unit Information
    "unitID": "drone_cairo",
    "groupID": "1",
    "unitDescription": "de_unit 1",
    
    // Connection Management
    "ping_server_rate_in_ms": 1500,
    "max_allowed_ping_delay_in_ms": 5000,
    "max_offline_count": 5,
    
    // Hardware Configuration
    "led_pins_enabled": true,
    "buzzer_pins_enabled": true,
    
    // Logging Configuration
    "logger_enabled": false,
    "logger_debug": false,
    
    // SSL Certificate (optional)
    "root_certificate_path": "./root.crt"
}
```

## Usage Throughout the Solution

### 1. **main.cpp** - Core Initialization
```cpp
de::CConfigFile& cConfigFile = de::CConfigFile::getInstance();
cConfigFile.initConfigFile(configName.c_str()); // "de_comm.config.module.json"
```

**Values accessed:**
- `module_id` - Module identification for UAVOS system
- `s2s_udp_packet_size` - UDP packet size for inter-module communication
- `s2s_udp_listening_ip` - IP address for UDP listener
- `s2s_udp_listening_port` - Port for UDP listener
- `groupID` - Group identification for module registration
- `unit_type` - Type of unit (control_unit/drone)
- `unitID` - Unit identifier (fallback from local config)
- `unitDescription` - Unit description (fallback from local config)
- `logger_enabled` - Enable/disable logging system
- `logger_debug` - Enable debug logging
- `led_pins_enabled` - Enable LED notification pins
- `buzzer_pins_enabled` - Enable buzzer notification pins
- `local_comm_server_ip` - Local communication server IP (optional)
- `local_comm_server_port` - Local communication server port (optional)

### 2. **andruav_auth.cpp** - Authentication System
```cpp
de::CConfigFile& cConfigFile = de::CConfigFile::getInstance();
const Json_de& jsonConfig = cConfigFile.GetConfigJSON();
```

**Values accessed:**
- `auth_ip` - Authentication server IP address
- `auth_port` - Authentication server port
- `userName` - Username for authentication
- `accessCode` - Access code/password
- `auth_verify_ssl` - SSL verification toggle
- `root_certificate_path` - Path to SSL certificate (optional)

**Usage context:**
- Building authentication URLs: `https://[auth_ip]:[auth_port]/api/endpoint`
- Configuring SSL settings for HTTPS requests
- Setting up certificate validation

### 3. **andruav_comm_server.cpp** - Remote Connection Management
```cpp
de::CConfigFile& cConfigFile = de::CConfigFile::getInstance();
const Json_de& jsonConfig = cConfigFile.GetConfigJSON();
```

**Values accessed:**
- `ping_server_rate_in_ms` - Ping interval in milliseconds (default: 1500ms)
- `max_allowed_ping_delay_in_ms` - Maximum delay before reconnection (default: 5000ms)
- `max_offline_count` - Maximum offline attempts before abort (default: 5)

**Usage context:**
- Watchdog thread configuration for connection monitoring
- Automatic reconnection logic
- Connection health monitoring

### 4. **andruav_comm_server_local.cpp** - Local Connection Management
```cpp
de::CConfigFile& cConfigFile = de::CConfigFile::getInstance();
const Json_de& jsonConfig = cConfigFile.GetConfigJSON();
```

**Values accessed:**
- Same as remote server: `ping_server_rate_in_ms`, `max_allowed_ping_delay_in_ms`, `max_offline_count`

**Usage context:**
- Local communication server watchdog
- Same monitoring logic as remote but for local connections

## Configuration Flow Logic

### Initialization Sequence
1. **Singleton Creation**: First call to `getInstance()` creates the instance
2. **File Loading**: `initConfigFile()` loads and parses the JSON
3. **File Monitoring**: `fileUpdated()` monitors for external changes
4. **Runtime Updates**: `updateJSON()` allows dynamic configuration changes

### File Monitoring Logic
```cpp
bool fileUpdated() {
    auto lastWriteTime = std::filesystem::last_write_time(m_file_url);
    if (lastWriteTime == m_lastWriteTime) return false;
    m_lastWriteTime = lastWriteTime;
    return true;
}
```

### Update Logic
The system supports nested key updates:
```cpp
// Update nested value
updateJSON('{"follow_me.quad.PID_P_X": 1.5}');
// Equivalent to:
// m_ConfigJSON["follow_me"]["quad"]["PID_P_X"] = 1.5;
```

## Error Handling

### Validation Pattern
```cpp
if (validateField(jsonConfig,"field_name", Json_de::value_t::type)) {
    value = jsonConfig["field_name"].get<type>();
} else {
    // Use default value or handle error
}
```

### Fatal Errors
- Missing required authentication fields (`auth_ip`, `auth_port`) → `exit(1)`
- Missing required network fields (`s2s_udp_listening_ip`, `s2s_udp_listening_port`) → Warning + default

### Graceful Degradation
- Optional fields use defaults when missing
- File monitoring errors logged but don't crash
- SSL certificate errors logged but continue with warnings

## Thread Safety

- **Singleton**: Thread-safe through static initialization
- **File Operations**: Protected by filesystem atomicity
- **JSON Access**: Read-only after initialization (except explicit updates)
- **Monitoring**: File timestamp comparison is atomic

## Integration Points

### With CLocalConfigFile
- `CConfigFile` handles global/module configuration
- `CLocalConfigFile` handles instance-specific overrides
- Local config takes precedence for certain fields (unitID, unitDescription)

### With Module Manager
- Provides network configuration for UDP databus
- Supplies module identification for registration
- Enables/disables hardware features (LEDs, buzzer)

### With Authentication System
- Supplies server connection parameters
- Configures SSL/TLS settings
- Provides user credentials

## Best Practices

1. **Always validate fields** before accessing to avoid runtime exceptions
2. **Use meaningful defaults** for optional configuration
3. **Monitor file changes** for runtime configuration updates
4. **Create backups** before saving configuration changes
5. **Handle nested structures** properly when updating configuration

## Dependencies

- **nlohmann/json**: JSON parsing and manipulation
- **std::filesystem**: File monitoring and operations
- **std::stringstream**: String manipulation for parsing
- **C++17**: Required for filesystem support

## File Locations

- **Header**: `src/configFile.hpp`
- **Implementation**: `src/configFile.cpp`
- **Configuration**: `de_comm.config.module.json`
- **Backups**: `de_comm.config.module.json.bak_YYYYMMDD_HHMMSS`

---

*This document provides a comprehensive overview of the CConfigFile singleton pattern usage and configuration management in the DroneEngage Communication Module.*
