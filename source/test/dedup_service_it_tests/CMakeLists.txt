

add_executable(FileServiceTests FileServiceTest.cpp DataBaseManagerTests.cpp)

target_link_libraries(FileServiceTests PRIVATE
        gtest gmock #[[gtest_main]]
        FileUtilsLib
)

file(COPY res DESTINATION ${CMAKE_BINARY_DIR}/source/test/files_service_it_tests)

add_test(NAME FileServiceTests COMMAND FileServiceTests)