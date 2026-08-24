# AGENTS.md — drone_engage_communication_pro

DroneEngage Communication broker (`de_comm`). The hub all vehicle modules
talk to over UDP; bridges modules to the Andruav servers/WebClients. C++17,
plugin-broker pattern. See the parent `../AGENTS.md` for the workspace-wide
architecture, `de_common` vendoring model, and config conventions — this
file only adds what's specific to this module.

## Build

    ./build.sh                 # DEBUG (default); accepts RELEASE + extra -D flags
    ./build_release.sh         # RELEASE
    ./build_ddebug.sh          # DEBUG + DDEBUG=ON

`build.sh` is the flexible one: `./build.sh RELEASE FOO=ON BAR` sets
`CMAKE_BUILD_TYPE=RELEASE`, passes `FOO=ON` to CMake, and `-DBAR` to the
compiler. Out-of-source in `build/` (in-source builds are blocked).
Binary: `bin/de_comm`.

### CMake options

- `DDEBUG` — detailed debug output.
- Auto-increment build number from `.version` on RELEASE builds
  (MAJOR.MINOR.BUGFIX.BUILD). Edit `MAJOR_VERSION`/`MINOR_VERSION`/
  `BUGFIX_VERSION` near the top of `CMakeLists.txt` to bump.
- CPack Debian packaging configured (`CPACK_PACKAGE_VERSION`).

### Dependencies

Boost 1.74+ (coroutine, context, thread, system, chrono), libcurl,
OpenSSL, plog (vendored in `3rdparty`), Threads. See `llms.txt` for the
full stack summary.

## Run

    ./start1.sh                # xterm launching bin/de_comm with config
    ./bin/de_comm -c ./de_comm.config.module.json

`start1.sh` / `start2.sh` / `start3.sh` launch separate instances.
`start_local_server.sh` runs a local server. `deployment/` holds
parent/slave multi-unit configs (`de_comm.config.module.parent.json`,
`...slave.json`) with `start_parent.sh` / `start_slave.sh`.

## Config

- `de_comm.config.module.json` — module config (WebClient UI).
- `template.json` — schema for the UI groups.
- `de_comm.local` — instance identity (`module_key`, `party_id`,
  `unitID`, `unitDescription`).

## Source layout

`src/` — `main.cpp`, `comm_server/`, `de_broker/` (plugin broker),
`de_general_mission_planner/`, `configFile.{cpp,hpp}`, `hal/` +
`hal_linux/`, `helpers/`, `notification_module/`, `3rdparty/`.
`src/de_common/` is the vendored `de_common` copy (see parent AGENTS.md).
