find_package(GTest REQUIRED)
find_package(libpqxx REQUIRED)

add_executable(inter_Tests tsts1.cpp
        dedup_service_it_tests/FileServiceTest.cpp
        dedup_service_it_tests/DataBaseManagerTests.cpp
        dedup_service_it_tests/testClasses.h)
target_link_libraries(inter_Tests gtest gmock gtest_main gmock_main dbUtilsLib)


add_subdirectory(clock_array_tests)
add_subdirectory(dedup_service_it_tests)
#[[todo link alter commonLibrary,FileUtilsLib,dbUtilsLib]]