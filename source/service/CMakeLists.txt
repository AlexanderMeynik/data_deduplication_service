

find_package(OpenSSL REQUIRED)

find_package(glog 0.7.1 REQUIRED)

find_package(libpqxx REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(/home/login/libpqxx/include)
link_directories(/home/login/libpqxx/src)

file(GLOB resourse CONFIGURE_DEPENDS
        "res/*.*"
)
file(COPY res DESTINATION ${CMAKE_BINARY_DIR})

file(GLOB common_files CONFIGURE_DEPENDS
        "common/*.cpp"
        "common/*.h"
)
file(GLOB FileUtils_files CONFIGURE_DEPENDS
        "FileUtils/*.cpp"
        "FileUtils/*.h"
)
file(GLOB dbUtils_files CONFIGURE_DEPENDS
        "dbUtils/*.cpp"
        "dbUtils/*.h"
)

add_library(commonLibrary ${common_files})
target_link_libraries(commonLibrary glog::glog)

add_library(dbUtilsLib ${dbUtils_files})
target_link_libraries(dbUtilsLib libpqxx::pqxx pq commonLibrary)

add_library(FileUtilsLib ${FileUtils_files})
target_link_libraries(FileUtilsLib OpenSSL::SSL -lcrypto commonLibrary dbUtilsLib)


add_executable(file_service fileServiceTest.cpp)
target_link_libraries(file_service FileUtilsLib)

add_executable(tsts1 main2.cpp)
target_link_libraries(tsts1 commonLibrary)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_target(run_file_service
            COMMAND ${CMAKE_COMMAND} -E env
            GLOG_log_dir=/mnt/c/projects/logs/dedup
            GLOG_logtostderr=0
            GLOG_stderrthreshold=0
            ./file_service
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS file_service
    )
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_target(run_file_service
            COMMAND ${CMAKE_COMMAND} -E env
            GLOG_log_dir=/mnt/c/projects/logs/dedup
            GLOG_logtostderr=0
            GLOG_stderrthreshold=2
            ./file_service
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS file_service
    )
endif ()



