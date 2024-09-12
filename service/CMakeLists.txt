

find_package(OpenSSL REQUIRED)

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

add_library(dbUtilsLib ${dbUtils_files})
target_link_libraries(dbUtilsLib -lpqxx -lpq commonLibrary)

add_library(FileUtilsLib ${FileUtils_files})
target_link_libraries(FileUtilsLib OpenSSL::SSL -lcrypto commonLibrary)


add_executable(data_deduplication_service main.cpp)
target_link_libraries(data_deduplication_service -lpqxx -lpq)

add_executable(SegmentTest main2.cpp)
target_link_libraries(SegmentTest FileUtilsLib)

add_executable(Segment_dataTest main3.cpp)
target_link_libraries(Segment_dataTest FileUtilsLib dbUtilsLib)