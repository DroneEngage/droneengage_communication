`CAndruavCommServer` is a singleton class managing WebSocket-based communication between a drone unit and a central server.  
It orchestrates connection lifecycle, message routing, and system-level commands within the DroneEngage communication framework.

---

### Definition

`CAndruavCommServer` is a concrete implementation of a communication server that uses WebSocket protocols to interface with remote parties (e.g., ground control stations, other drones). It inherits from both `CCallBack_WSASession` and `CAndruavCommServerBase`, indicating it handles low-level WebSocket events and provides high-level communication APIs.

The class follows the **singleton pattern**, ensuring only one instance exists throughout the application. This is critical for centralized communication management in embedded or drone systems where resource coordination is essential.

```cpp
13:62:/mnt/8a619ce7-cd3f-4520-af65-7991f16410f7/public_versions/drone_engage/drone_engage_communication_pro/src/comm_server/andruav_comm_server.hpp
class CAndruavCommServer : public CCallBack_WSASession, public CAndruavCommServerBase
{
public:
    static CAndruavCommServer& getInstance()
    {
        static CAndruavCommServer instance;
        return instance;
    };

private:
    CAndruavCommServer() : CAndruavCommServerBase() {}
    CAndruavCommServer(CAndruavCommServer const&) = delete;
    void operator=(CAndruavCommServer const&) = delete;

public:
    ~CAndruavCommServer() override = default;

    // Connection lifecycle
    void start() override;
    void connect() override;
    void uninit(const bool exit) override;

    // Event callbacks
    void onSocketError() override;
    void onBinaryMessageRecieved(const char*, std::size_t) override;
    void onTextMessageRecieved(const std::string&) override;

    // API methods
    void API_pingServer() override;
    void API_sendSystemMessage(const int, const Json_de&) const override;
    void API_sendCMD(const std::string&, const int, const Json_de&) override;
    std::string API_sendCMDDummy(...) override;
    void API_sendBinaryCMD(...) override;
    void sendMessageToCommunicationServer(...) override;

private:
    void startWatchDogThread() override;
    void connectToCommServer(const std::string&, const std::string&, const std::string&) override;

private:
    de::andruav_servers::CWSAProxy& _cwsa_proxy = CWSAProxy::getInstance();
    std::unique_ptr<CWSASession> _cwsa_session;

    CAndruavUnits& m_andruav_units = CAndruavUnits::getInstance();
    de::comm::CAndruavMessage& m_andruav_message = CAndruavMessage::getInstance();
};
```

- **Type**: Singleton class
- **Inheritance**: `CCallBack_WSASession`, `CAndruavCommServerBase`
- **Scope**: `de::andruav_servers`
- **Key Members**:
  - `_cwsa_proxy`: Reference to a WebSocket abstraction layer proxy
  - `_cwsa_session`: Unique pointer managing active WebSocket session
  - `m_andruav_units`: Registry of connected drone units
  - `m_andruav_message`: Message factory/processor singleton
- **Thread Safety**: Implicitly assumed via singleton pattern and internal state management
- **Side Effects**: Manages network connections, spawns watchdog threads, sends/receives messages

---

### Example Usages

The `CAndruavCommServer` is accessed globally via its `getInstance()` method. It is used in core modules such as `main.cpp` and `de_modules_manager.cpp` to initiate communication, send commands, and monitor connection status.

#### Sending a System Message

This real-world example shows how a system message is dispatched using the communication server:

```cpp
278:294:/home/mhefny/TDisk/public_versions/drone_engage/drone_engage_communication_pro/src/comm_server/andruav_facade.cpp
CAndruavCommServer::getInstance().API_sendSystemMessage(
    TYPE_AndruavSystem_LoadTasks, 
    message
);
```

> Sends a `LoadTasks` system command to reload mission tasks from the server. This is typically triggered during initialization or configuration reload.

#### Checking Connection Status in Module Manager

Another usage occurs in the module manager, where the current connection status is embedded into inter-module messages:

```cpp
182:/home/mhefny/TDisk/public_versions/drone_engage/drone_engage_communication_pro/src/de_broker/de_modules_manager.cpp
ms[JSON_INTERMODULE_SOCKET_STATUS] = andruav_servers::CAndruavCommServer::getInstance().getStatus();
```

> Injects the current socket status (e.g., connected/disconnected) into module-to-module status updates, enabling synchronized behavior across components.

#### Turning Off Communication

In message parsing logic, the server can be programmatically disabled:

```cpp
393:/home/mhefny/TDisk/public_versions/drone_engage/drone_engage_communication_pro/src/comm_server/andruav_parser.cpp
de::andruav_servers::CAndruavCommServer::getInstance().turnOnOff(false, 0);
```

> Disables the communication server — likely part of a handover or fail-safe mechanism.

Overall, `CAndruavCommServer` is **widely used** across:
- `main.cpp`: Initialization and main loop integration
- `andruav_facade.cpp`: High-level command routing
- `de_modules_manager.cpp`: Status reporting
- `andruav_parser.cpp`: Incoming message handling

It serves as a central hub for all external communications.

---

### Notes

- **Non-blocking design**: Despite not explicitly showing async code, the use of callbacks like `onTextMessageRecieved` implies event-driven, non-blocking I/O — essential for real-time drone operations.
- **Dependency on WSA (WebSocket Abstraction)**: The class relies heavily on `CWSAProxy` and `CWSASession`, suggesting a custom or encapsulated WebSocket stack designed for reliability in mobile/embedded environments.
- **Tight integration with unit registry**: By holding a reference to `CAndruavUnits`, it can route messages to specific drone units, enabling multi-drone coordination.

---

### See Also

- `CAndruavCommServerBase`: Abstract base class defining the interface; `CAndruavCommServer` implements these virtual methods to provide concrete behavior.
- `CCallBack_WSASession`: Provides low-level WebSocket event hooks; allows `CAndruavCommServer` to react to socket errors and incoming data.
- `CAndruavUnits`: Global registry of drone units; used by `CAndruavCommServer` to manage peer identities and routing.
- `CAndruavMessage`: Message serialization/deserialization utility; used when formatting outgoing commands or parsing incoming ones.
- `CWSAProxy` and `CWSASession`: Core WebSocket communication components; handle actual socket I/O and connection management underneath.