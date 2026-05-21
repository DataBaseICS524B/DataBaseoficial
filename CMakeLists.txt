cmake_minimum_required(VERSION 3.15)
project(CustomDB VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/core/catalog)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/core/storage)

find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

# остальное без изменений...

find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

# Статическая библиотека клиента
add_library(customdb_client STATIC
    src/lib/DatabaseClient.cpp
    src/lib/Connection.cpp
    src/lib/client.cpp
    src/core/catalog/Column.cpp
    src/core/catalog/Table.cpp
    src/core/catalog/Database.cpp
    src/core/catalog/Catalog.cpp
    src/core/storage/StorageEngine.cpp
    src/core/storage/Index.cpp
)

target_link_libraries(customdb_client PRIVATE nlohmann_json::nlohmann_json)

target_compile_definitions(customdb_client PRIVATE CUSTOMDB_BUILD_DLL)

# Сервер
add_executable(customdb_server
    src/cli/main.cpp
    src/network/Server.cpp
    src/network/ThreadPool.cpp
    src/network/ClientSession.cpp
    src/network/Protocol.cpp
)

target_link_libraries(customdb_server PRIVATE customdb_client nlohmann_json::nlohmann_json Threads::Threads)

# Тесты
#add_executable(simple_tests tests/test_catalog.cpp)
#add_executable(full_test tests/full_test.cpp)

#target_link_libraries(simple_tests PRIVATE customdb_client nlohmann_json::nlohmann_json)
#target_link_libraries(full_test PRIVATE customdb_client nlohmann_json::nlohmann_json)

# Установка
install(TARGETS customdb_client customdb_server
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)



option(BUILD_TESTS "Build integration tests" ON)

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()