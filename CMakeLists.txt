cmake_minimum_required(VERSION 3.20)
project(TGVerity VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(tgverity_core
    src/tgverity/relay_packet.cpp
    src/tgverity/p2p_frame.cpp
    src/tgverity/tg_client_stub.cpp
)

target_include_directories(tgverity_core PUBLIC src)

add_executable(tgverity src/main.cpp)
target_link_libraries(tgverity PRIVATE tgverity_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(relay_packet_tests tests/relay_packet_tests.cpp)
    target_link_libraries(relay_packet_tests PRIVATE tgverity_core)
    add_test(NAME relay_packet_tests COMMAND relay_packet_tests)
endif()
