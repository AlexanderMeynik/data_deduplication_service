cmake_minimum_required(VERSION 3.26)
project(data_deduplication_service)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(test)

#[[if(BUILD_TEST)
    add_subdirectory(test)
endif()]]


find_package(OpenSSL REQUIRED)

find_package(glog 0.7.1 REQUIRED)

find_package(OpenCV REQUIRED)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(include)

add_library(commonLibrary src/myconcepts.cpp src/ClockArray.cpp)
target_link_libraries(commonLibrary glog::glog)

add_library(dbUtilsLib src/dbManager.cpp src/lib.cpp)
target_link_libraries(dbUtilsLib pqxx pq commonLibrary)

add_library(FileUtilsLib src/ServiceFileInterface.cpp)
target_link_libraries(FileUtilsLib OpenSSL::SSL -lcrypto commonLibrary dbUtilsLib)#[[remove lcrypto]]


add_executable(file_service src/fileServiceTest.cpp)
target_link_libraries(file_service FileUtilsLib)