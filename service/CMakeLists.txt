add_executable(data_deduplication_service main.cpp)
target_link_libraries(data_deduplication_service -lpqxx -lpq)
file(GLOB resourse CONFIGURE_DEPENDS
        "res/*.*"
)
file(COPY res DESTINATION ${CMAKE_BINARY_DIR})