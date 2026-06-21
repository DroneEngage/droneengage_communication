# DroneEngage Mesh Setup

This document describes how to build a DroneEngage mesh using a local drone server on or near a drone, a super server, and multiple drone servers connected under the same relay tree.

## Goal

The mesh allows a remote drone to join the rest of the DroneEngage system even when it cannot connect directly to the public/main communication server.

A drone connects to a local drone server. That drone server forwards messages to a super server. The super server can have multiple drone servers connected to it, and each drone server can manage a group of local drones or control units.

## Mesh Topology

```
                         ┌──────────────────────┐
                         │     Super Server     │
                         │  andruav_server S2S  │
                         │  enable_super_server │
                         └──────────┬───────────┘
                                    │
              ┌─────────────────────┼─────────────────────┐
              │                     │                     │
              ▼                     ▼                     ▼
    ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
    │ Drone Server A  │  │ Drone Server B  │  │ Drone Server C  │
    │ local server    │  │ local server    │  │ local server    │
    │ relay to super  │  │ relay to super  │  │ relay to super  │
    └────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘
             │                     │                     │
             ▼                     ▼                     ▼
      Local drones /        Local drones /        Local drones /
      GCS modules           GCS modules           GCS modules
```

A remote drone can carry or reach one of the drone servers. Its local communication module connects to that drone server using `use_local_comm_server=true`. The drone server then relays the drone messages to the super server, which propagates them to other drone servers and connected clients.

## Components

| Component | Project | Role |
|-----------|---------|------|
| Super Server | `andruav_server` | Parent relay server. Accepts child drone-server connections. |
| Drone Server | `andruav_server` | Local server for a drone group. Connects to the super server as a persistent relay child. |
| Communication Module | `drone_engage_communication_pro` | Drone or control-unit client. Connects either to auth/main server or directly to a local drone server. |
| Drone Modules | `drone_engage_mavlink` and other modules | Local unit modules connected through UDP databus to the communication module. |

## Server-to-Server Message Flow

```
Remote Drone Module
       │
       │ local websocket connection
       ▼
Local Drone Server
       │
       │ persistent S2S relay
       ▼
Super Server
       │
       ├──► Other Drone Server A
       ├──► Other Drone Server B
       └──► Other Drone Server C
```

The `andruav_server` relay layer forwards messages between local WebSocket clients and server-to-server connections. Messages from local clients are processed by `fn_parseMessage()` and forwarded using `forwardMessage()`. Messages received from another server are processed by `fn_parseExternalMessage()` and may be re-forwarded using `forwardExternalMessage()`.

Loop prevention depends on a unique `server_id`. Relay messages include `_origin`, and a server drops messages that originated from itself.

## Super Server Configuration

Use a server config based on `andruav_server/deployment/server.s2s.super.config`.

Important fields:

| Field | Value | Purpose |
|-------|-------|---------|
| `server_id` | unique name, for example `SuperServer` | Used for loop prevention. |
| `server_port` | public/client WebSocket port | Optional client access to the super server. |
| `enable_super_server` | `true` | Accepts drone-server relay connections. |
| `s2s_super_server_ip` | bind IP | IP used by the S2S parent listener. |
| `s2s_super_server_port` | relay port, for example `9866` | Port drone servers connect to. |
| `enable_persistant_relay` | `false` | The top super server does not connect to another parent. |
| `local_server_enabled` | `false` | Super server normally uses auth/main server behavior. |

Run example:

```bash
node server.js --config=deployment/server.s2s.super.config
```

## Drone Server Configuration

Use a server config based on `andruav_server/deployment/server.s2s.drone.config`.

Important fields:

| Field | Value | Purpose |
|-------|-------|---------|
| `server_id` | unique drone-server name | Must be different per drone server. |
| `server_port` | local client WebSocket port, for example `9967` | Communication modules connect here. |
| `enable_super_server` | `false` | This drone server is not the parent in the mesh. |
| `enable_persistant_relay` | `true` | Connects this drone server to the super server. |
| `s2s_relay_to_super_server_ip` | super server IP | Parent relay target. |
| `s2s_relay_to_super_server_port` | super server relay port | Parent relay target port. |
| `local_server_enabled` | `true` | Allows local drone modules to connect without external auth. |
| `ignore_auth_server` | `true` | Local-only drone server mode. |
| `local_server_account_id` | account ID | Account context for local clients. |

Run example:

```bash
node server.js --config=deployment/server.s2s.drone.config
```

For multiple drone servers, create one config per drone server and change at least:

- `server_id`
- `server_port`
- `public_host` if clients need a specific address
- `s2s_relay_to_super_server_ip`
- `s2s_relay_to_super_server_port` if the super relay port changes
- SSL certificate paths if each server has its own certificate

## Communication Module Configuration on the Drone

The communication module can connect directly to a local drone server instead of using the auth server.

Use a config based on `drone_engage_communication_pro/de_comm.config.module.localserver.json` or `drone_engage_communication_pro/deployment/de_comm.config.module.slave.json`.

Important fields:

| Field | Value | Purpose |
|-------|-------|---------|
| `use_local_comm_server` | `true` | Enables direct connection to the local drone server. |
| `local_comm_server_ip` | drone server IP | IP address reachable by the drone. |
| `local_comm_server_port` | drone server WebSocket port | Usually the drone server `server_port`. |
| `unitID` | unique drone/control-unit ID | Identifies this unit in the group. |
| `groupID` | mesh group ID | Must match the intended DroneEngage group. |
| `s2s_udp_listening_ip` | local UDP bind IP | Databus listener for local modules. |
| `s2s_udp_listening_port` | local UDP port | Must not conflict with another module on the same host. |

The communication module startup logic uses local-server mode only when `use_local_comm_server` is `true` and both `local_comm_server_ip` and `local_comm_server_port` exist. Otherwise it falls back to auth-server/main-server mode.

Run example:

```bash
./de_comm --config=deployment/de_comm.config.module.slave.json
```

## Minimal Setup Procedure

### 1. Start the Super Server

On the machine that acts as the relay root:

```bash
node server.js --config=deployment/server.s2s.super.config
```

Confirm that the S2S parent listener is reachable by all drone servers.

### 2. Start Each Drone Server

On each drone-server machine:

```bash
node server.js --config=deployment/server.s2s.drone.config
```

Each drone server should connect to the super server using `enable_persistant_relay=true` and the configured `s2s_relay_to_super_server_*` fields.

### 3. Start Drone Communication Modules

On each drone or companion computer:

```bash
./de_comm --config=deployment/de_comm.config.module.slave.json
```

Set `local_comm_server_ip` to the local drone server address. For a drone carrying its own server, this can be `127.0.0.1` or the local LAN interface. For a nearby drone server, use the reachable network address.

### 4. Verify Message Propagation

Use two units connected to different drone servers in the same `groupID`.

Expected behavior:

- A message from Drone Server A is delivered locally on A.
- The message is forwarded to the Super Server.
- The Super Server forwards it to other drone servers.
- Other drone servers deliver it to local clients in the same group.
- The original server does not process its own relayed message again because `_origin` prevents loops.

## Remote Drone Scenario

A remote drone can be integrated by running an `andruav_server` drone-server instance on the drone companion computer or on a nearby reachable gateway.

```
Remote Drone
   ├── de_comm
   ├── local MAVLink/modules over UDP databus
   └── local andruav_server drone-server
              │
              │ long-range IP link / VPN / LTE / radio IP bridge
              ▼
          Super Server
              │
              └── remaining DroneEngage mesh
```

In this mode:

- The drone modules only need to reach the local drone server.
- The local drone server handles the long-range relay to the super server.
- Other drone groups remain reachable through the super server.
- If the long-range link drops, local drone modules can continue talking to the local drone server and reconnect to the mesh when the relay returns.

## Operational Notes

- `server_id` must be unique for every `andruav_server` instance in the mesh.
- All drone servers that should share data must use compatible account/group settings.
- `groupID` controls message visibility at the DroneEngage protocol level.
- Do not reuse the same `server_port` or UDP databus port on the same host.
- Expose only the required ports between drone servers and the super server.
- If SSL is enabled, ensure each client trusts the certificate or `allow_fake_SSL` is enabled for the intended test environment.
- For multi-hop-like deployment, keep the relay tree clear: one super server as parent, multiple drone servers as children.

## Related Files

| File | Purpose |
|------|---------|
| `andruav_server/wiki/MessagePropagation.md` | Server-to-server relay behavior and loop prevention. |
| `andruav_server/deployment/server.s2s.super.config` | Example super-server config. |
| `andruav_server/deployment/server.s2s.drone.config` | Example drone-server config. |
| `drone_engage_communication_pro/src/main.cpp` | Communication module local-server selection logic. |
| `drone_engage_communication_pro/de_comm.config.module.localserver.json` | Example communication module local-server config. |
| `drone_engage_communication_pro/deployment/de_comm.config.module.slave.json` | Example communication module config for a slave/drone node. |
