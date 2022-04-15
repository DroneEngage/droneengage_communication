

file(GLOB folder_src "./*.cpp")
file(GLOB folder_helpers "./helpers/*.cpp")
file(GLOB folder_common "./comm_server/*.cpp")
file(GLOB hal "./hal/*.cpp")
file(GLOB hal_linux "./hal_linux/*.cpp")
file(GLOB notification_module "./notification_module/*.cpp")
file(GLOB uavos "./uavos/*.cpp")

set(files ${folder_src} ${folder_common} ${folder_helpers}
${hal} ${hal_linux} ${notification_module} ${uavos} )



add_executable( OUTPUT_BINARY ${files})
set_target_properties( OUTPUT_BINARY 
                PROPERTIES 
                    OUTPUT_NAME "de_comm"
                    SUFFIX ".so"
                )
target_link_libraries(OUTPUT_BINARY  ${Boost_LIBRARIES} OpenSSL::Crypto OpenSSL::SSL CURL::libcurl Threads::Threads )
