enable_testing()


find_package(GTest REQUIRED)
find_package(libpqxx REQUIRED)
find_package(glog 0.7.1 REQUIRED)

include_directories(../include)
include_directories(../fff)
file(GLOB TEST_SOURCES_1 clock_array/*.cpp)
add_executable(clock_array_tests ${TEST_SOURCES_1})


add_executable(benchmark bencmark.cpp)
target_link_libraries(benchmark
        FileUtilsLib gtest gmock
)


target_link_libraries(clock_array_tests PRIVATE
        gtest gmock gtest_main
        commonLibrary
)

file(GLOB TEST_SOURCES_2 files_service_IT/*.cpp)
add_executable(FileServiceTests ${TEST_SOURCES_2})

target_link_libraries(FileServiceTests PRIVATE
        gtest gmock FileUtilsLib
)


file(COPY test_data DESTINATION ${CMAKE_BINARY_DIR})


gtest_discover_tests(FileServiceTests)