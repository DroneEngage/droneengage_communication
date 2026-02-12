cmake_minimum_required(VERSION 3.10.0)

if(NOT WIN32)
  string(ASCII 27 Esc)
  SET(ColourReset "${Esc}[m")
  SET(ColourBold  "${Esc}[1m")
  SET(Red         "${Esc}[31m")
  SET(Green       "${Esc}[32m")
  SET(Yellow      "${Esc}[33m")
  SET(Blue        "${Esc}[34m")
  SET(Magenta     "${Esc}[35m")
  SET(Cyan        "${Esc}[36m")
  SET(White       "${Esc}[37m")
  SET(BoldRed     "${Esc}[1;31m")
  SET(BoldGreen   "${Esc}[1;32m")
  SET(BoldYellow  "${Esc}[1;33m")
  SET(BoldBlue    "${Esc}[1;34m")
  SET(BoldMagenta "${Esc}[1;35m")
  SET(BoldCyan    "${Esc}[1;36m")
  SET(BoldWhite   "${Esc}[1;37m")
endif()


if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
message ("${Red} In-source builds not allowed! Create a build directory and run CMake from there.  ${ColourReset}")
message( FATAL_ERROR “In-source builds not allowed! Create a build directory and run CMake from there. ” )
exit(1)
endif()

# Auto-increment version on each build
set(VERSION_FILE "${CMAKE_SOURCE_DIR}/.version")

# Manually set major, minor, and bugfix versions (you can change these)
set(MAJOR_VERSION 3)
set(MINOR_VERSION 10)
set(BUGFIX_VERSION 0)

if(EXISTS ${VERSION_FILE})
    file(READ ${VERSION_FILE} CURRENT_VERSION)
    string(STRIP "${CURRENT_VERSION}" CURRENT_VERSION)
    # Extract current version components
    string(REPLACE "." ";" VERSION_LIST ${CURRENT_VERSION})
    list(LENGTH VERSION_LIST LIST_LENGTH)
    if(LIST_LENGTH EQUAL 4)
        list(GET VERSION_LIST 0 STORED_MAJOR)
        list(GET VERSION_LIST 1 STORED_MINOR)
        list(GET VERSION_LIST 2 STORED_BUGFIX)
        list(GET VERSION_LIST 3 STORED_BUILD)
        
        # Check if major, minor, or bugfix version changed
        if(NOT (${STORED_MAJOR} EQUAL ${MAJOR_VERSION} AND ${STORED_MINOR} EQUAL ${MINOR_VERSION} AND ${STORED_BUGFIX} EQUAL ${BUGFIX_VERSION}))
            set(BUILD_VERSION 0)  # Reset build when major/minor/bugfix changes
            message(STATUS "Version changed from ${STORED_MAJOR}.${STORED_MINOR}.${STORED_BUGFIX}.${STORED_BUILD} to ${MAJOR_VERSION}.${MINOR_VERSION}.${BUGFIX_VERSION}, resetting build to 0")
        else()
            set(BUILD_VERSION ${STORED_BUILD})  # Continue with existing build
        endif()
    else()
        set(BUILD_VERSION 0)  # Default build version
    endif()
else()
    set(BUILD_VERSION 0)  # Default build version
endif()

# Increment build version only for RELEASE builds
IF(CMAKE_BUILD_TYPE MATCHES RELEASE)
    math(EXPR BUILD_VERSION "${BUILD_VERSION} + 1")
    message(STATUS "Incrementing build version for RELEASE build")
ELSE()
    message(STATUS "Non-release build - keeping current build version")
ENDIF()

# Create new version string
set(__APP__VERSION__ "${MAJOR_VERSION}.${MINOR_VERSION}.${BUGFIX_VERSION}.${BUILD_VERSION}")

message(STATUS "Building version: ${__APP__VERSION__}")
message(STATUS "To change major/minor/bugfix versions, edit MAJOR_VERSION, MINOR_VERSION, and BUGFIX_VERSION above")

SET (__APP__VERSION__ "${__APP__VERSION__}")
add_definitions( -D__APP__VERSION__="${__APP__VERSION__}")
project(DE_COMMUNICATOR_PRO VERSION "${__APP__VERSION__}")




SET (CMAKE_C_COMPILER /usr/bin/gcc)
SET (CMAKE_CXX_COMPILER /usr/bin/g++)

#C++ Version
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED True)


find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

# Boost Library 

#set(Boost_NO_SYSTEM_PATHS TRUE)
#set(Boost_NO_BOOST_CMAKE TRUE)

SET(Boost_USE_STATIC_LIBS   OFF)
SET(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_USE_MULTITHREADED ON)

SET (BOOST_MIN_VERSION "1.74.0")
FIND_PACKAGE(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS coroutine context thread system chrono)

if(Boost_USE_STATIC_LIBS)
   message(STATUS "Linking against ${Yellow}boost static libraries${ColourReset}")
else()
   message(STATUS "Linking against ${Yellow}boost dynamic libraries${ColourReset}")
endif()
message(STATUS "Setting up BOOST")
message(STATUS "Includes - ${Yellow}${Boost_INCLUDE_DIRS}${ColourReset}")
message(STATUS "Library  - ${Yellow}${Boost_LIBRARY_DIRS}${ColourReset}")
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# Define an option for user control over DETAILED_DEBUG
option(DDEBUG "Detailed Debug" OFF) # Default is OFF

#define default build
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
SET(CMAKE_BUILD_TYPE DEBUG)
ENDIF()


message ("${Yellow}=========================================================================${ColourReset}")
message ("CMAKE_C_COMPILER: "  ${CMAKE_C_COMPILER})
message ("CMAKE_CXX_COMPILER: " ${CMAKE_CXX_COMPILER})

#deceide debug or release
IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
message("CMAKE_BUILD_TYPE: ${BoldYellow} DEBUG ${ColourReset}")
ELSEIF (CMAKE_BUILD_TYPE MATCHES RELEASE)
message("CMAKE_BUILD_TYPE: ${BoldGreen} RELEASE ${ColourReset}")
ENDIF()

# adding CMAKE_CXX_FLAGS properly
add_compile_options(
  -Wno-return-type 
  -Wno-address-of-packed-member
  -Werror=unused-variable
  -Werror=unused-result
  $<$<CONFIG:DEBUG>:-g3>
  $<$<CONFIG:DEBUG>:-Og>
  $<$<CONFIG:DEBUG>:-ggdb>
  $<$<CONFIG:RELEASE>:-O2>
  $<$<CONFIG:RELEASE>:-Werror=parentheses>
)

# adding DEFINEs
add_compile_definitions(
    $<$<CONFIG:DEBUG>:DEBUG>
    $<$<CONFIG:RELEASE>:RELEASE>
)


set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-export-dynamic")

# define output directory to generate binary
SET(OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
file(MAKE_DIRECTORY ${OUTPUT_DIRECTORY})
SET(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIRECTORY})

add_subdirectory(src)

# Conditionally define DDEBUG based on the DDEBUG option
target_compile_definitions(OUTPUT_BINARY PRIVATE
    $<$<BOOL:${DDEBUG}>:DDEBUG>
)

configure_file(de_comm.config.module.json ${OUTPUT_DIRECTORY}/de_comm.config.module.json COPYONLY)
configure_file(root.crt ${OUTPUT_DIRECTORY}/root.crt COPYONLY)
configure_file(template.json ${OUTPUT_DIRECTORY}/template.json COPYONLY)

file(GLOB _SCRIPT_FILES "${PROJECT_SOURCE_DIR}/scripts/*")
foreach(_SCRIPT_FILE IN LISTS _SCRIPT_FILES)
  get_filename_component(_SCRIPT_NAME "${_SCRIPT_FILE}" NAME)
  configure_file("${_SCRIPT_FILE}" "${OUTPUT_DIRECTORY}/scripts/${_SCRIPT_NAME}" COPYONLY)
endforeach()
# Highlight if DDEBUG or TEST_MODE_NO_HAILO_LINK are enabled
if (DDEBUG)
    message(STATUS "${Red}Option DDEBUG is ENABLED.${ColourReset}")
endif()

message ("${Yellow}=========================================================================${ColourReset}")

# Installation configuration for Debian package only
install(TARGETS OUTPUT_BINARY
    RUNTIME DESTINATION usr/bin
    COMPONENT runtime
)

install(FILES 
    ${PROJECT_SOURCE_DIR}/de_comm.config.module.json
    ${PROJECT_SOURCE_DIR}/root.crt
    ${PROJECT_SOURCE_DIR}/template.json
    DESTINATION etc/drone_engage/de_communicator
    COMPONENT runtime
)

# Install scripts directory
install(DIRECTORY ${PROJECT_SOURCE_DIR}/scripts/
    DESTINATION usr/share/drone_engage/de_communicator/scripts
    COMPONENT runtime
)

# CPack configuration for Debian package generation
set(CPACK_PACKAGE_NAME "de-communicator-pro")
set(CPACK_PACKAGE_VERSION "${__APP__VERSION__}")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6, libssl-dev, libcurl4-openssl-dev")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "DroneEngage Team")
set(CPACK_DEBIAN_PACKAGE_SECTION "misc")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DroneEngage Communication Pro Module")

# Optional description file if it exists
if(EXISTS "${PROJECT_SOURCE_DIR}/README.md")
    set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
endif()

include(CPack)

# Set CPack for proper Debian packaging
set(CPACK_SET_DESTDIR TRUE)
set(CPACK_INSTALL_PREFIX "/")

# Write version file after successful RELEASE build
IF(CMAKE_BUILD_TYPE MATCHES RELEASE)
    file(WRITE "${VERSION_FILE}" "${__APP__VERSION__}")
    message(STATUS "Version file updated: ${VERSION_FILE} = ${__APP__VERSION__}")
ENDIF()

# Generate build success script for dependency report
set(BUILD_SUCCESS_SCRIPT "${CMAKE_BINARY_DIR}/build_success.cmake")
file(WRITE "${BUILD_SUCCESS_SCRIPT}" "")
file(APPEND "${BUILD_SUCCESS_SCRIPT}" "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E echo \"\")\n")
file(APPEND "${BUILD_SUCCESS_SCRIPT}" "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E cmake_echo_color --green --bold \"=========================================================================\")\n")
file(APPEND "${BUILD_SUCCESS_SCRIPT}" "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E cmake_echo_color --green --bold \"BUILD COMPLETED SUCCESSFULLY\")\n")
file(APPEND "${BUILD_SUCCESS_SCRIPT}" "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E cmake_echo_color --cyan --bold \"Version: ${__APP__VERSION__}\")\n")
file(APPEND "${BUILD_SUCCESS_SCRIPT}" "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E cmake_echo_color --cyan --bold \"Build Number: ${BUILD_VERSION}\")\n")
file(APPEND "${BUILD_SUCCESS_SCRIPT}" "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E cmake_echo_color --cyan --bold \"Dependency Files: ${CMAKE_BINARY_DIR}/src/CMakeFiles/OUTPUT_BINARY.dir/*.d\")\n")
file(APPEND "${BUILD_SUCCESS_SCRIPT}" "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E cmake_echo_color --green --bold \"=========================================================================\")\n")
file(APPEND "${BUILD_SUCCESS_SCRIPT}" "execute_process(COMMAND \"${CMAKE_COMMAND}\" -E echo \"\")\n")

# Add custom target to run the build success script
add_custom_target(build_report ALL
    COMMAND ${CMAKE_COMMAND} -P "${BUILD_SUCCESS_SCRIPT}"
    DEPENDS OUTPUT_BINARY
)

