find_package(GTest REQUIRED)

add_executable(inter_Tests tsts1.cpp)
target_link_libraries(inter_Tests gtest gmock gtest_main gmock_main)

