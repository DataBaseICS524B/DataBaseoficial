add_executable(integration_tests
    integration/test_catalog_storage.cpp
)

target_link_libraries(integration_tests
    PRIVATE
        customdb_client
        GTest::gtest_main
        nlohmann_json::nlohmann_json
)

target_include_directories(integration_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Не используем gtest_discover_tests, просто добавляем тест в CTest (опционально)
enable_testing()
add_test(NAME integration_tests COMMAND integration_tests)

# Unit tests
add_executable(unit_tests
    unit/unit_tests_all.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        customdb_client
        GTest::gtest_main
        nlohmann_json::nlohmann_json
)

target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(NAME unit_tests COMMAND unit_tests)