cmake_minimum_required(VERSION 3.10)

project(ConcurrentServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.66 REQUIRED COMPONENTS system thread)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/server
    ${Boost_INCLUDE_DIRS}
)

set(SOURCE_FILES
    main.cpp
    session.cpp
    http_server.cpp
    request_handler.cpp
    file_manager.cpp
    performance_monitor.cpp
)

add_executable(ConcurrentServer ${SOURCE_FILES})

target_link_libraries(ConcurrentServer
    ${Boost_LIBRARIES}
    pthread
)