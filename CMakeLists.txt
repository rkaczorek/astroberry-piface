cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi-piface CXX C)

set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")


find_package(INDI REQUIRED)

#configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})

################ PiFace Board ################

set(indi_piface_relay_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/piface_relay.cpp
   )

add_executable(indi_piface_relay ${indi_piface_relay_SRCS})
target_link_libraries(indi_piface_relay indidriver mcp23s17)
install(TARGETS indi_piface_relay RUNTIME DESTINATION bin )
install(FILES indi_piface_relay.xml DESTINATION ${INDI_DATA_DIR})

################ PiFace Focuser ################

set(indi_piface_focuser_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/piface_focuser.cpp
   )

add_executable(indi_piface_focuser ${indi_piface_focuser_SRCS})
target_link_libraries(indi_piface_focuser indidriver mcp23s17)
install(TARGETS indi_piface_focuser RUNTIME DESTINATION bin )
install(FILES indi_piface_focuser.xml DESTINATION ${INDI_DATA_DIR})

################ PiFace CAD ################

set(indi_piface_cad_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/piface_cad.cpp
   )

add_executable(indi_piface_cad ${indi_piface_cad_SRCS})
target_link_libraries(indi_piface_cad indidriver ${CMAKE_CURRENT_SOURCE_DIR}/libpifacecad.a ${CMAKE_CURRENT_SOURCE_DIR}/libmcp23s17.a)
install(TARGETS indi_piface_cad RUNTIME DESTINATION bin )
install(FILES indi_piface_cad.xml DESTINATION ${INDI_DATA_DIR})

