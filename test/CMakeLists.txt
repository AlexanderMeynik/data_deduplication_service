find_package(GTest REQUIRED)
find_package(libpqxx REQUIRED)
include_directories(../include)
include_directories(../fff)
file(GLOB TEST_SOURCES_1 clock_array/*.cpp)
add_executable(clock_array_tests ${TEST_SOURCES_1}
        testUtils.h)

target_link_libraries(clock_array_tests PRIVATE
        gtest gmock gtest_main
        commonLibrary
)

file(GLOB TEST_SOURCES_2 files_service_IT/*.cpp)
add_executable(FileServiceTests ${TEST_SOURCES_2})

target_link_libraries(FileServiceTests PRIVATE
        gtest gmock #[[gtest_main]] dbUtilsLib
)

add_test(NAME FileServiceTests COMMAND FileServiceTests)
add_test(NAME clock_array_tests COMMAND clock_array_tests)

file(COPY test_data DESTINATION ${CMAKE_BINARY_DIR})