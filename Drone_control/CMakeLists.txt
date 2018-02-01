cmake_minimum_required(VERSION 2.8.12)

project(voice_droneControl)

if(NOT MSVC)
    add_definitions("-std=c++11 -Wall -Wextra -Werror")
else()
    add_definitions("-std=c++11 -WX -W2")
    add_definitions("-D_USE_MATH_DEFINES") # For M_PI
    include_directories(${CMAKE_SOURCE_DIR}/../../install/include)
    link_directories(${CMAKE_SOURCE_DIR}/../../install/lib)
endif()

add_executable(voice_droneControl
    voice_droneControl.cpp
)

target_link_libraries(voice_droneControl
    dronecore
)
