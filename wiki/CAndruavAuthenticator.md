`CAndruavAuthenticator` is a singleton class responsible for authenticating drone agents and validating hardware with a remote server.  
It handles secure login and hardware verification using HTTPS requests, parsing JSON responses to configure communication parameters.

---

### Definition

`CAndruavAuthenticator` is a C++ class defined in the `de::andruav_servers` namespace, designed to manage authentication and hardware validation for drone agents connecting to a communication server. It uses a singleton pattern to ensure only one instance exists, centralizing authentication state and data.

The class performs two main operations:
- User authentication via username and access code (`doAuthentication`)
- Hardware ID validation against a server (`doValidateHardware`)

It communicates over HTTPS using `libcurl`, parses responses with `nlohmann::json`, and stores configuration such as the communication server IP, port, and temporary login key.

```cpp
57:135:/mnt/8a619ce7-cd3f-4520-af65-7991f16410f7/public_versions/drone_engage/drone_engage_communication_pro/src/comm_server/andruav_auth.hpp
class CAndruavAuthenticator
{
public:
    static CAndruavAuthenticator& getInstance()
    {
        static CAndruavAuthenticator instance;
        return instance;
    };

    CAndruavAuthenticator(CAndruavAuthenticator const&) = delete;
    void operator=(CAndruavAuthenticator const&) = delete;

private:
    CAndruavAuthenticator() {}

public:
    ~CAndruavAuthenticator() {}

    bool isAuthenticationOK() { return m_is_authentication_ok; }
    bool doAuthentication();
    bool doValidateHardware(const std::string hardware_id, const int hardware_type);
    void uninit();

    const int& getErrorCode() { return m_auth_error; }
    const std::string& getErrorString() { return m_auth_error_string; }

public:
    std::string m_comm_server_ip;
    int         m_comm_server_port;
    std::string m_comm_server_key;

private:
    bool getAuth(std::string url, std::string param, std::string& response);
    bool getAuth_doAuthentication(std::string url, std::string param);
    bool getAuth_doValidateHardware(std::string url, std::string param);
    std::string stringifyError(const int& error_number);
    void translateResponse_doAuthentication(const std::string& response);
    bool translateResponse_doValidateHardware(const std::string& response);

private:
    std::string m_access_code;
    std::string m_permissions;
    std::string m_agent = "d";
    int m_auth_error = 0;
    std::string m_auth_error_string;
    std::string m_hardware_error_string;
    bool m_is_authentication_ok = false;
};
```

- **Type**: Class (`class CAndruavAuthenticator`)
- **Pattern**: Singleton (only one instance allowed via `getInstance()`)
- **Key Methods**:
  - `doAuthentication()`: Authenticates agent using credentials from config file
  - `doValidateHardware(hardware_id, hardware_type)`: Validates drone hardware ID with server
- **Side effects**: Modifies internal state (`m_comm_server_ip`, `m_comm_server_port`, etc.) on successful auth
- **Returns**: Boolean success/failure; error details via `getErrorCode()` and `getErrorString()`
- **Dependencies**: `libcurl` for HTTP(S), `nlohmann::json` for parsing, config file via `CConfigFile`

---

### Example Usages

In `andruav_comm_server.cpp`, `CAndruavAuthenticator` is used to authenticate before establishing a WebSocket connection. This ensures only authorized drones can connect to the communication server.

```cpp
149:158:/home/mhefny/TDisk/public_versions/drone_engage/drone_engage_communication_pro/src/comm_server/andruav_comm_server.cpp
CAndruavAuthenticator& andruav_auth = CAndruavAuthenticator::getInstance();

m_status = SOCKET_STATUS_CONNECTING;
if (!andruav_auth.doAuthentication() || !andruav_auth.isAuthenticationOK())   
{
    m_status = SOCKET_STATUS_ERROR;
    PLOG(plog::error) << "Communicator Server Connection Status: SOCKET_STATUS_ERROR"; 
    de::comm::CUavosModulesManager::getInstance().handleOnAndruavServerConnection(m_status);
    return;
}
```

This usage pattern shows:
- Singleton instance accessed via `getInstance()`
- Authentication performed before connection attempt
- Immediate failure handling if auth fails or is not OK

**Overall Usage Summary**:
- Defined in `andruav_auth.hpp`, implemented in `andruav_auth.cpp`
- Used primarily in `andruav_comm_server.cpp` during connection setup
- No other callers found — tightly scoped to communication initialization
- Central to security: blocks connection unless authentication succeeds

---

### Notes

- Despite being named `CAndruavAuthenticator`, it authenticates *drone agents*, not users directly — the term "agent" refers to the drone-side software component.
- The class uses hardcoded URL paths like `/agent/al/` and `/agent/ah/`, defined via macros (`AUTH_AGENT_LOGIN_COMMAND`, `AUTH_AGENT_HARDWARE_COMMAND`), which are constructed into full HTTPS URLs using config-provided server IP.
- It relies on a configuration file (`CConfigFile`) to read `auth_ip`, `auth_port`, account name, and access code — missing fields cause fatal errors and immediate exit.

---

### See Also

- `CConfigFile`: Provides configuration data including authentication server address and credentials; required for `doAuthentication()` to function.
- `AUTH_AGENT_LOGIN_COMMAND` (`"/agent/al/"`): The URL endpoint used for login, combined with parameters like `acc=`, `sid=`, and `&pwd=`.
- `doValidateHardware()`: Used to verify a drone's hardware ID (e.g., CPU ID) against a server database, likely preventing unauthorized or cloned devices from connecting.
- `translateResponse_doAuthentication()`: Parses JSON response from auth server, extracting `cs` (communication server), `g` (public host), `h` (port), and `f` (temp key) fields to populate instance variables.
