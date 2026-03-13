# FindSystemBoost.cmake - Prefer system Boost over Anaconda/conda environments
# This fixes static initialization issues caused by mixing system and conda libraries

# First, try to find system Boost using standard methods
# Set CMAKE_FIND_ROOT_PATH to prefer system directories
set(BOOST_SEARCH_PATHS
    /usr
    /usr/local
    /opt
    /opt/local
)

# Clear any conda-related paths from search
if(DEFINED ENV{CONDA_PREFIX})
    list(REMOVE_ITEM CMAKE_SYSTEM_PREFIX_PATH "$ENV{CONDA_PREFIX}")
    list(REMOVE_ITEM CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}")
endif()

# Find Boost with system preference
find_package(Boost 1.74.0 REQUIRED COMPONENTS 
    coroutine 
    context 
    thread 
    system 
    chrono
)

# Verify we're not using conda/anaconda libraries
if(Boost_LIBRARIES)
    foreach(lib ${Boost_LIBRARIES})
        if(lib MATCHES "anaconda|conda")
            message(WARNING "Found conda/anaconda Boost library: ${lib}")
            message(WARNING "This may cause static initialization issues")
            message(WARNING "Consider installing system Boost: sudo apt-get install libboost-all-dev")
        endif()
    endforeach()
endif()

# Set variables for use in parent scope
set(Boost_LIBRARIES ${Boost_LIBRARIES} PARENT_SCOPE)
set(Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} PARENT_SCOPE)
