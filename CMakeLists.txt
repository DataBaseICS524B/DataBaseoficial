cmake_minimum_required(VERSION 3.15)
project(CustomDB VERSION 1.0.0)

# Устанавливаем C++ стандарт
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Определяем тип сборки
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Включаем папки с заголовками
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Путь к nlohmann/json
find_package(nlohmann_json REQUIRED)

# Создаём статическую библиотеку (вместо динамической для упрощения)
add_library(customdb_client STATIC
    src/lib/DatabaseClient.cpp
    src/lib/Connection.cpp
    src/lib/client.cpp
    src/core/catalog/Column.cpp
    src/core/catalog/Table.cpp
    src/core/catalog/Database.cpp
    src/core/catalog/Catalog.cpp
    src/core/storage/StorageEngine.cpp
)

# Линкуем nlohmann/json
target_link_libraries(customdb_client PRIVATE nlohmann_json::nlohmann_json)

# Определяем макрос
target_compile_definitions(customdb_client PRIVATE CUSTOMDB_BUILD_DLL)

# Тесты - линкуем со статической библиотекой
add_executable(simple_tests
    tests/test_catalog.cpp
)

# Полные тесты
add_executable(full_test tests/full_test.cpp)
target_link_libraries(full_test PRIVATE customdb_client)

target_link_libraries(simple_tests PRIVATE customdb_client nlohmann_json::nlohmann_json)

# Установка
install(TARGETS customdb_client
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)