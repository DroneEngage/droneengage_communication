# DE Comm Architecture: Plugin-Broker Interaction

## Overview

The DroneEngage Communication (DE Comm) system consists of two main components that communicate via UDP:

1. **Plugin Side** - Uses the `de_comm` library
2. **Broker Side** - The `de_comm` broker module

This document explains the architecture, communication flow, and key interactions between these components.

## Architecture Components

### Plugin Side (de_comm library)

#### Core Classes
- **`CModule`** (`de_module.hpp/cpp`) - Main interface for plugins
- **`CUDPClient`** (`udpClient.hpp/cpp`) - UDP communication layer  
- **`CFacade_Base`** (`de_facade_base.cpp`) - High-level API wrapper
- **`CAndruavMessageParserBase`** (`de_message_parser_base.cpp`) - Message parsing

#### Key Features
- Singleton pattern for `CModule`
- Thread-safe UDP communication with chunking
- Message routing based on destination and type
- Periodic module identification broadcasts

### Broker Side (de_comm broker module)

#### Core Classes
- **`CUavosModulesManager`** (`de_modules_manager.cpp`) - Central module manager
- **`CUDPCommunicator`** (`udpCommunicator.cpp`) - UDP server for modules
- **`CMessageBuffer`** (`andruav_message_buffer.cpp`) - Thread-safe message queue

#### Key Features
- Module registry and lifecycle management
- Message routing and forwarding
- License validation
- Camera device management
- Thread-safe message processing

## Communication Flow

### 1. Module Registration

The registration process establishes the connection between plugins and the broker:

```
Plugin (CModule) → UDP → Broker (CUavosModulesManager)
```

#### Steps:
1. **Plugin Initialization**
   ```cpp
   // Plugin calls
   CModule::defineModule(class, id, key, version, filter);
   CModule::init(targetIP, broadcastPort, host, listenPort, chunkSize);
   ```

2. **ID Broadcast**
   ```cpp
   // Plugin sends identification
   CModule::createJSONID(true);  // Request reply
   ```

3. **Broker Processing**
   - Broker receives via `CUDPCommunicator::InternalReceiverEntry()`
   - Processes in `CUavosModulesManager::handleModuleRegistration()`
   - Maintains module registry in `m_modules_list`

4. **Broker Response**
   ```cpp
   // Broker replies with its identification
   CUavosModulesManager::createJSONID(false);
   ```

### 2. Message Types & Routing

#### Inter-Module Messages
- **Routing Type**: `CMD_TYPE_INTERMODULE`
- **Purpose**: Communication between plugins only
- **Flow**: Plugin → Broker → Other Plugins (not forwarded to external server)
- **Use Case**: Plugin coordination, internal commands

#### System Messages  
- **Routing Type**: `CMD_COMM_SYSTEM`
- **Purpose**: Plugin-to-Broker system commands
- **Flow**: Plugin → Broker (handled internally)
- **Use Case**: Configuration, status updates, system control

#### Group/Individual Messages
- **Routing Type**: `CMD_COMM_GROUP` or `CMD_COMM_INDIVIDUAL`
- **Purpose**: Messages to be forwarded to external DroneEngage server
- **Flow**: Plugin → Broker → External Server
- **Use Case**: External communication, telemetry, commands

### 3. Message Structure

#### Module Identification Message
```json
{
    "ty": "uv",                    // Inter-module routing
    "1": 9100,                     // TYPE_AndruavModule_ID
    "ms": {
        "a": "module_id",          // Module identifier
        "b": "module_class",       // Module type (fcb, camera, etc.)
        "c": [1005, 1041],        // Subscribed message types
        "d": ["T", "R"],          // Module features
        "e": "module_key",         // Unique module key
        "s": "hardware_serial",    // Hardware identifier
        "t": 1,                    // Hardware type
        "z": true,                 // Resend request flag
        "v": "1.0.0",             // Module version
        "i": 1234567890           // Instance timestamp
    }
}
```

#### Standard Message
```json
{
    "ty": "uv",                    // Routing type
    "0": "target_id",             // Target module/group
    "1": message_type,            // Message type ID
    "k": "module_key",            // Sender module key
    "ms": {                       // Message payload
        // Message-specific data
    }
}
```

## Thread Architecture

### Plugin Side Threads

#### Main Thread
- Application logic
- Message sending via `CModule::sendJMSG()`, `sendBMSG()`

#### UDP Receiver Thread
```cpp
CUDPClient::InternalReceiverEntry()
```
- Receives UDP packets
- Handles chunked message reassembly
- Calls `onReceive()` callback

#### ID Sender Thread  
```cpp
CUDPClient::InternelSenderIDEntry()
```
- Periodic module identification broadcasts
- 1-second intervals
- Ensures broker knows module is alive

### Broker Side Threads

#### Main Thread
- Module management
- Message routing decisions
- Status tracking

#### UDP Receiver Thread
```cpp
CUDPCommunicator::InternalReceiverEntry()
```
- Receives UDP packets from modules
- Handles chunked message reassembly
- Queues messages for processing

#### Consumer Thread
```cpp
CUavosModulesManager::consumerThreadFunc()
```
- Processes queued messages
- Calls `parseIntermoduleMessage()`
- Handles message routing and forwarding

## Module Management

### Module Registry
The broker maintains several data structures:

#### Active Modules
```cpp
std::map<std::string, std::unique_ptr<MODULE_ITEM_TYPE>> m_modules_list;
```
- Registry of connected plugins
- Contains module metadata, socket addresses, status

#### Message Subscriptions
```cpp
std::map<int, std::vector<std::string>> m_module_messages;
```
- Maps message types to subscribing modules
- Enables efficient message routing

#### Camera Registry
```cpp
std::map<std::string, std::unique_ptr<std::map<std::string, std::unique_ptr<MODULE_CAMERA_ENTRY>>>> m_camera_list;
```
- Special handling for camera modules
- Tracks available camera devices

### Module Classes
Supported module types:
- `MODULE_CLASS_FCB` - Flight Control Board
- `MODULE_CLASS_VIDEO` - Camera modules
- `MODULE_CLASS_P2P` - Peer-to-Peer communication
- `MODULE_CLASS_GPIO` - GPIO control
- `MODULE_CLASS_TRACKING` - Object tracking
- `MODULE_CLASS_A_RECOGNITION` - AI recognition
- `MODULE_CLASS_SDR` - Software Defined Radio
- `MODULE_CLASS_SOUND` - Audio processing

### Module Features
Modules declare capabilities:
- `T` - Transmit telemetry
- `R` - Receive telemetry  
- `C` - Capture images
- `V` - Capture video
- `G` - GPIO control
- `A` - AI recognition
- `K` - Object tracking
- `P` - P2P communication

## UDP Communication

### Chunking Protocol
Large messages are split into chunks for reliable UDP transmission:

#### Chunk Structure
```
[2 bytes: chunk number] [chunk data]
```

#### Chunk Numbers
- `0` to `N-1`: Sequential chunks
- `0xFFFF`: Last chunk marker

#### Reassembly Process
1. Receive chunks with same source port
2. Store in order using chunk number
3. When `0xFFFF` received, concatenate all chunks
4. Add null terminator for text messages
5. Pass to message processor

### Error Handling
- Comprehensive try-catch blocks throughout
- Graceful degradation on communication failures
- Module restart detection via timestamp comparison
- Socket cleanup and thread termination

## Security & Licensing

### License Validation
```cpp
void CUavosModulesManager::checkLicenseStatus(MODULE_ITEM_TYPE* module_item)
```

#### Process:
1. Check if authentication is available
2. Validate hardware serial and type
3. Set license status:
   - `LICENSE_VERIFIED_OK` - Module allowed
   - `LICENSE_VERIFIED_BAD` - Module blocked
   - `LICENSE_NOT_VERIFIED` - Authentication unavailable
4. Send error notification if invalid

#### Hardware Types
- `HARDWARE_TYPE_CPU` - Standard CPU-based modules
- `HARDWARE_TYPE_UNDEFINED` - Unspecified hardware

## Message Processing

### Plugin Side Processing
```cpp
void CModule::onReceive(const char* message, int len)
```

#### Steps:
1. Parse JSON message
2. Validate required fields
3. Handle special messages:
   - `TYPE_AndruavModule_ID` - Module identification
   - `TYPE_AndruavMessage_DUMMY` - Test messages
4. Call user-defined callback for other messages

### Broker Side Processing
```cpp
void CUavosModulesManager::parseIntermoduleMessage(const char* full_message, const std::size_t full_message_length, const struct sockaddr_in* ssock)
```

#### Steps:
1. Parse JSON message
2. Determine message type and routing
3. Handle special message types via switch statement
4. Route based on message type:
   - Inter-module: Forward to other modules
   - System: Handle internally
   - Other: Forward to external server

## Key Interactions

### Module Discovery
1. Plugin broadcasts identification
2. Broker registers module
3. Broker replies with its identification
4. Plugin stores party_id and group_id

### Message Routing
1. Plugin sends message with routing type
2. Broker determines destination based on routing
3. Broker forwards accordingly:
   - To other modules (inter-module)
   - To external server (group/individual)
   - Handles internally (system)

### Health Monitoring
1. Modules send periodic ID broadcasts
2. Broker tracks last access time
3. Broker marks modules as dead if no updates
4. Broker updates system status based on alive modules

## Configuration

### Plugin Configuration
```cpp
CModule::defineModule(
    "module_class",           // Module type
    "module_id",             // Module identifier  
    "unique_key",            // Unique module key
    "1.0.0",                 // Module version
    [1001, 1002]             // Subscribed message types
);
```

### Network Configuration
```cpp
CModule::init(
    "127.0.0.1",             // Target IP (broker)
    60000,                   // Broker port
    "0.0.0.0",               // Local bind address
    60001,                   // Local port
    8192                     // UDP chunk size
);
```

## Best Practices

### Plugin Development
1. Use singleton `CModule::getInstance()` for communication
2. Set message callback via `setMessageOnReceive()`
3. Handle both JSON and binary messages appropriately
4. Implement proper error handling
5. Use appropriate routing types for different message categories

### Broker Development
1. Use thread-safe data structures
2. Implement proper message validation
3. Handle module restarts gracefully
4. Monitor module health and update status
5. Use message buffering for high-throughput scenarios

## Debugging

### Logging
Both components use colored console output:
- `_INFO_CONSOLE_TEXT` - Informational messages
- `_SUCCESS_CONSOLE_TEXT` - Success messages  
- `_ERROR_CONSOLE_TEXT` - Error messages
- `_LOG_CONSOLE_TEXT` - Debug messages

### Common Issues
1. **UDP Port Conflicts** - Ensure unique ports for each module
2. **Message Size** - Use chunking for large messages
3. **Thread Safety** - Use proper mutex protection
4. **Network Connectivity** - Verify broker is reachable
5. **Module Registration** - Check module key uniqueness

## Conclusion

The DE Comm architecture provides a robust, scalable communication system enabling:

- **Plugin Coordination** - Direct inter-module communication
- **External Integration** - Forwarding to DroneEngage servers
- **System Management** - Centralized module lifecycle management
- **Security** - License validation and access control
- **Reliability** - Chunked UDP, error handling, health monitoring

This design supports complex drone systems with multiple specialized modules communicating efficiently through a central broker.