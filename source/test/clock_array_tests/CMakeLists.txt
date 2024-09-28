
add_executable(clock_array_tests clock_array_test.cpp
        clock_array_interface_tests.cpp
        commont.h)

target_link_libraries(clock_array_tests PRIVATE
        gtest gmock gtest_main
        commonLibrary
)


add_test(NAME ClockArrayTest COMMAND clock_array_tests)