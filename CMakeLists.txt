cmake_minimum_required(VERSION 3.26)
project(data_deduplication_service)

set(CMAKE_CXX_STANDARD 23)

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

add_library(commonLibrary src/myConcepts.cpp src/clockArray.cpp src/HashUtils.cpp)
target_link_libraries(commonLibrary glog::glog OpenSSL::SSL -lcrypto)

add_library(dbUtilsLib src/dbManager.cpp src/dbCommon.cpp)
target_link_libraries(dbUtilsLib pqxx pq commonLibrary)

add_library(FileUtilsLib src/FileService.cpp)
target_link_libraries(FileUtilsLib commonLibrary dbUtilsLib)


add_executable(file_service src/fileServiceTest.cpp)
target_link_libraries(file_service FileUtilsLib)

add_executable(ssltest src/main.cpp)
target_link_libraries(ssltest FileUtilsLib commonLibrary FileUtilsLib)

option(BUILD_DOC "Build documentation" OFF)
if(BUILD_DOC)
    add_subdirectory(documentation)
endif (BUILD_DOC)