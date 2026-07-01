cmake_minimum_required(VERSION 3.20)
project(TGVerity VERSION 0.1.0 LANGUAGES CXX)

option(TGVERITY_USE_TDLIB "Build Telegram client against TDLib tdjson" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(tgverity_core
    src/app/config.cpp
    src/tgverity/bridge.cpp
    src/tgverity/relay_packet.cpp
    src/tgverity/p2p_frame.cpp
    src/tgverity/p2p_socket.cpp
)

target_include_directories(tgverity_core PUBLIC src)

if(TGVERITY_USE_TDLIB)
    find_package(Td CONFIG REQUIRED)
    target_sources(tgverity_core PRIVATE src/tg/td_client.cpp)
    target_link_libraries(tgverity_core PRIVATE Td::TdJson)
    target_compile_definitions(tgverity_core PRIVATE TGVERITY_USE_TDLIB=1)
else()
    target_sources(tgverity_core PRIVATE src/tg/td_client.cpp src/tgverity/tg_client_stub.cpp)
endif()

add_executable(tgverity src/main.cpp)
target_link_libraries(tgverity PRIVATE tgverity_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(relay_packet_tests tests/relay_packet_tests.cpp)
    target_link_libraries(relay_packet_tests PRIVATE tgverity_core)
    add_test(NAME relay_packet_tests COMMAND relay_packet_tests)

    add_executable(app_bridge_tests tests/app_bridge_tests.cpp)
    target_link_libraries(app_bridge_tests PRIVATE tgverity_core)
    add_test(NAME app_bridge_tests COMMAND app_bridge_tests)
endif()
