cmake_minimum_required(VERSION 3.20)
project(TGVerity VERSION 0.2.0 LANGUAGES CXX)

option(TGVERITY_USE_TDLIB "Build Telegram client against TDLib tdjson" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(tgverity_core
    src/app/config.cpp
    src/tgverity/log.cpp
    src/tgverity/crypto_provider.cpp
    src/tgverity/state_machine.cpp
    src/tgverity/session_store.cpp
    src/tgverity/relay_packet.cpp
    src/tgverity/bridge.cpp
    src/tgverity/p2p_frame.cpp
    src/tgverity/p2p_socket.cpp
    src/tgverity/tg_client_stub.cpp
    src/tgverity/tg_client_tdlib.cpp
    src/tgverity/fake_telegram_client.cpp
    src/tg/td_client.cpp
)

target_include_directories(tgverity_core PUBLIC src)

if(TGVERITY_USE_TDLIB)
    find_package(Td CONFIG REQUIRED)
    target_link_libraries(tgverity_core PRIVATE Td::TdJson)
    target_compile_definitions(tgverity_core PRIVATE TGVERITY_USE_TDLIB=1)
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

    add_executable(log_tests tests/log_tests.cpp)
    target_link_libraries(log_tests PRIVATE tgverity_core)
    add_test(NAME log_tests COMMAND log_tests)

    add_executable(crypto_provider_tests tests/crypto_provider_tests.cpp)
    target_link_libraries(crypto_provider_tests PRIVATE tgverity_core)
    add_test(NAME crypto_provider_tests COMMAND crypto_provider_tests)

    add_executable(state_machine_tests tests/state_machine_tests.cpp)
    target_link_libraries(state_machine_tests PRIVATE tgverity_core)
    add_test(NAME state_machine_tests COMMAND state_machine_tests)

    add_executable(session_store_tests tests/session_store_tests.cpp)
    target_link_libraries(session_store_tests PRIVATE tgverity_core)
    add_test(NAME session_store_tests COMMAND session_store_tests)

    add_executable(bridge_tests tests/bridge_tests.cpp)
    target_link_libraries(bridge_tests PRIVATE tgverity_core)
    add_test(NAME bridge_tests COMMAND bridge_tests)
endif()
