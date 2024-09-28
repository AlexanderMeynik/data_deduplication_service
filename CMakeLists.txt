cmake_minimum_required(VERSION 3.26)
project(source/service)

set(CMAKE_CXX_STANDARD 23)

#[[file(GLOB common_files CONFIGURE_DEPENDS
        "IT/*.cpp"
        "IT/*.h"
)]]
add_subdirectory(source/service)
add_subdirectory(source/test)