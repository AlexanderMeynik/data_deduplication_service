

find_package(OpenSSL REQUIRED)
list(APPEND CMAKE_PREFIX_PATH "/home/login/glog/build")
list(APPEND CMAKE_PREFIX_PATH "/home/login/libpqxx/build")

find_package(glog  0.7.1 REQUIRED)

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


add_executable(create_db_test main.cpp)
target_link_libraries(create_db_test dbUtilsLib)

add_executable(SegmentTest main2.cpp)
target_link_libraries(SegmentTest FileUtilsLib)

add_executable(Segment_dataTest main3.cpp)
target_link_libraries(Segment_dataTest FileUtilsLib dbUtilsLib)

add_executable(create_insert_retrieve db_create_data_write_test.cpp)
target_link_libraries(create_insert_retrieve FileUtilsLib dbUtilsLib)

add_executable(file_hierarhy_create pathing.cpp)
target_link_libraries(file_hierarhy_create glog::glog)
add_executable(pcv opcvtest.cpp)
target_link_libraries(pcv ${OpenCV_LIBS})


add_executable(file_service fileServiceTest.cpp)
target_link_libraries(file_service FileUtilsLib )

