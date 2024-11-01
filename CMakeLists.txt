cmake_minimum_required(VERSION 3.26)
project(data_deduplication_service)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET( CMAKE_CXX_FLAGS_DEBUG  "-g -std=gnu++2b")
SET( CMAKE_CXX_FLAGS_RELEASE  "-O3 -ffast-math -DNDEBUG -march=native -fpic -ftree-vectorize")
SET( CMAKE_CXX_FLAGS  " -std=gnu++2b -fcolor-diagnostics")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_subdirectory(test)


find_package(libpqxx REQUIRED)
find_package(glog 0.7.1 REQUIRED)
find_package(OpenSSL REQUIRED)




set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)

add_library(commonLibrary lib/myConcepts.cpp lib/clockArray.cpp lib/HashUtils.cpp)
target_link_libraries(commonLibrary glog::glog OpenSSL::SSL -lcrypto)

add_library(dbUtilsLib lib/dbManager.cpp lib/dbCommon.cpp)
target_link_libraries(dbUtilsLib pqxx pq commonLibrary)

add_library(FileUtilsLib lib/FileService.cpp lib/fileUtils.cpp)
target_link_libraries(FileUtilsLib commonLibrary dbUtilsLib)


add_executable(file_service src/exampl/fileServiceTest.cpp)
target_link_libraries(file_service FileUtilsLib)

add_executable(ssltest src/exampl/main.cpp)
target_link_libraries(ssltest FileUtilsLib commonLibrary )

option(BUILD_DOC "Build documentation" OFF)
if(BUILD_DOC)
    add_subdirectory(doc)
endif (BUILD_DOC)


option(BUILD_GUI "Build GUI" OFF)
if(BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Xml Core Charts Sql)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    file(GLOB GUI_SOURCES src/*.cpp)
    add_executable(qt_main ${GUI_SOURCES})

    target_link_libraries(qt_main  Qt6::Widgets Qt6::Core Qt6::Xml Qt6::Gui Qt6::Sql Qt6::Charts FileUtilsLib)
endif (BUILD_GUI)