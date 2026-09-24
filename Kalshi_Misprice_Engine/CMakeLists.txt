cmake_minimum_required(VERSION 3.15)

project(Kalshi_Misprice_Engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

function(configure_target target_name)
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /O2 /W4)
    else()
        target_compile_options(${target_name} PRIVATE -O3 -Wall -Wextra)
    endif()
endfunction()

add_library(simdjson_impl STATIC simdjson.cpp)
configure_target(simdjson_impl)
target_include_directories(simdjson_impl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(engine main.cpp)
configure_target(engine)
target_link_libraries(engine PRIVATE simdjson_impl Threads::Threads)

add_executable(quickstart market_data_quickstart.cpp)
configure_target(quickstart)
target_link_libraries(quickstart PRIVATE simdjson_impl CURL::libcurl)

add_executable(test_ob tests/test_orderbook.cpp)
configure_target(test_ob)

add_executable(test_arb tests/test_arbitragedetect.cpp)
configure_target(test_arb)

add_executable(test_rb tests/test_ringbuffer.cpp)
configure_target(test_rb)

add_executable(test_kp tests/test_kalshiparser.cpp)
configure_target(test_kp)
target_link_libraries(test_kp PRIVATE simdjson_impl)

enable_testing()
add_test(NAME orderbook COMMAND test_ob)
add_test(NAME arbitragedetector COMMAND test_arb)
add_test(NAME ringbuffer COMMAND test_rb)
add_test(NAME kalshiparser COMMAND test_kp)