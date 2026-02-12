
[![Ardupilot Cloud EcoSystem](https://cloud.ardupilot.org/_static/ardupilot_logo.png  "Ardupilot Cloud EcoSystem")](https://cloud.ardupilot.org  "Ardupilot Cloud EcoSystem") **Drone Engage** is part of Ardupilot Cloud Eco System

  

------------

  

![Drone Engage Communicator Module](https://raw.githubusercontent.com/DroneEngage/droneengage_communication/master/resources/de_logo_title.png)

  

#Drone Engage Communicator Module

  

# What is Drone-Engage ?

  

Drone-Engage (D.E.) is a distibuted system that allows monitoring & controlling drones via Internet. Drone-Engage objective is to provide a Linux-based alternative for [Andruav](http://https://play.google.com/store/apps/details?id=arudpilot.andruav&hl=en&gl=US  "Andruav") on Android mobiles.

  

# Communication Module

  

This repository is for **Drone-Engage Communication Module**. This module runs on the vehicle and communicates with other modules such as [Mavlink_Module "de_mavlink"](https://github.com/HefnySco/de_mavlink  "Mavlink_Module") and Video_Module. On the other side it communicates with Andruav Server. This module receives messages from all plugins and forward them to [WebClients](https://github.com/HefnySco/andruav_webclient  "WebClients") and other vehicles, and receives commands from Webclients via Internet Server and process them or forward them to attached modules to process them.

  

For more Information Please Goto [Cloud.Ardupilot.org](https://cloud.ardupilot.org/  "Cloud.Ardupilot.org")

  
  
  
  

# Build the Code

## Prerequisites

### System Dependencies
```bash
sudo apt update
sudo apt install git cmake build-essential
sudo apt install libcurl4-openssl-dev libssl-dev
```

### Boost Library (Required)
The project requires Boost 1.74.0 or higher. You have two options:

#### Option 1: Install from Package Manager (Recommended)
```bash
sudo apt install libboost-all-dev
```

#### Option 2: Build from Source
```bash
cd ~
mkdir boost && cd boost
wget https://archives.boost.io/release/1.86.0/source/boost_1_86_0.tar.bz2
tar -xvjf boost_1_86_0.tar.bz2
cd boost_1_86_0
./bootstrap.sh
./b2
sudo ./b2 install
```

## Building the Project

### Clone and Build
```bash
# Clone the repository
git clone https://github.com/DroneEngage/droneengage_communication.git
cd droneengage_communication

# Clean previous build (if exists)
rm -rf ./build
mkdir build && cd build

# Configure CMake
cmake -D CMAKE_BUILD_TYPE=RELEASE -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON ../

# Build the project
make -j$(nproc)  # Use all available CPU cores

# Generate Debian package (optional)
cpack
```

### Build Types
- **DEBUG**: Includes debug symbols and additional logging
  ```bash
  cmake -D CMAKE_BUILD_TYPE=DEBUG ../
  ```
- **RELEASE**: Optimized build with version increment
  ```bash
  cmake -D CMAKE_BUILD_TYPE=RELEASE ../
  ```

### Additional Build Options
- **Detailed Debug**: Enable additional debug output
  ```bash
  cmake -D CMAKE_BUILD_TYPE=DEBUG -DDDEBUG=ON ../
  ```

## Installation

### From Debian Package (Recommended)
```bash
# Install the generated .deb package
sudo dpkg -i de-communicator-pro-*.deb
```

### Manual Installation
The build process creates the binary in `bin/de_comm`. You can copy it manually:
```bash
# Create deployment directory
mkdir -p $HOME/drone_engage/de_comm

# Copy binary and configuration files
cp bin/de_comm $HOME/drone_engage/de_comm/
cp de_comm.config.module.json template.json $HOME/drone_engage/de_comm/

# Copy scripts if they exist
if [ -d "scripts" ]; then
    cp -r scripts $HOME/drone_engage/de_comm/
fi
```

## Build Output

After successful build, you'll see:
```
=========================================================================
BUILD COMPLETED SUCCESSFULLY
Version: 3.10.0.x
Build Number: x
Dependency Files: /path/to/build/src/CMakeFiles/OUTPUT_BINARY.dir/*.d
=========================================================================
```

The binary will be available at `bin/de_comm` and Debian packages in the `build/` directory.
