cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi-astroberry-piface CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake_modules/")
include(GNUInstallDirs)

set (VERSION_MAJOR 2)
set (VERSION_MINOR 2)

find_package(INDI REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_piface_focuser.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_piface_focuser.xml)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_piface_relay.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_piface_relay.xml)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/libs/)

include(CMakeCommon)

add_library(mcp23s17 STATIC IMPORTED)
set_property(TARGET mcp23s17 PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/libs/libmcp23s17.a)

set(PIFACE_LIBRARIES "mcp23s17")

################ PiFace Focuser ################
set(indi_piface_focuser_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/piface_focuser.cpp
   )

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(indi_piface_focuser indi_piface_focuser_SRCS 6 cpp)
ENDIF ()

add_executable(indi_piface_focuser ${indi_piface_focuser_SRCS})
target_link_libraries(indi_piface_focuser ${INDI_DRIVER_LIBRARIES} ${PIFACE_LIBRARIES})
install(TARGETS indi_piface_focuser RUNTIME DESTINATION bin )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_piface_focuser.xml DESTINATION ${INDI_DATA_DIR})

################ PiFace Relay ################
set(indi_piface_relay_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/piface_relay.cpp
   )

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(indi_piface_relay indi_piface_relay_SRCS 6 cpp)
ENDIF ()

add_executable(indi_piface_relay ${indi_piface_relay_SRCS})
target_link_libraries(indi_piface_relay ${INDI_DRIVER_LIBRARIES} ${PIFACE_LIBRARIES})
install(TARGETS indi_piface_relay RUNTIME DESTINATION bin )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_piface_relay.xml DESTINATION ${INDI_DATA_DIR})
