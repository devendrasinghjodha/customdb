cmake_minimum_required(VERSION 3.20)
project(customdb VERSION 0.1.0 LANGUAGES CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(customdb_core
    src/api/database.cpp
    src/security/auth.cpp
    src/storage/page.cpp
    src/storage/pager.cpp
    src/storage/buffer_pool.cpp
    src/index/btree.cpp
    src/index/secondary_index.cpp
    src/mvcc/version.cpp
    src/query/query.cpp
    src/query/plan.cpp
    src/wal/wal.cpp
    src/transaction/lock_manager.cpp
    src/transaction/transaction.cpp
)

target_include_directories(customdb_core PUBLIC src)
target_compile_features(customdb_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(customdb_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(customdb_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(customdb src/main.cpp)
target_link_libraries(customdb PRIVATE customdb_core)

add_executable(customdb_benchmark benchmarks/benchmark.cpp)
target_link_libraries(customdb_benchmark PRIVATE customdb_core)

add_executable(customdb_smoke_tests tests/smoke_test.cpp)
target_link_libraries(customdb_smoke_tests PRIVATE customdb_core)
add_test(NAME customdb_smoke_tests COMMAND customdb_smoke_tests)

add_executable(customdb_integration_tests tests/integration_test.cpp)
target_link_libraries(customdb_integration_tests PRIVATE customdb_core)
add_test(NAME customdb_integration_tests COMMAND customdb_integration_tests)

add_executable(customdb_concurrency_tests tests/concurrency_test.cpp)
target_link_libraries(customdb_concurrency_tests PRIVATE customdb_core)
add_test(NAME customdb_concurrency_tests COMMAND customdb_concurrency_tests)

add_executable(customdb_recovery_tests tests/recovery_test.cpp)
target_link_libraries(customdb_recovery_tests PRIVATE customdb_core)
add_test(NAME customdb_recovery_tests COMMAND customdb_recovery_tests)

if(WIN32)
    target_compile_definitions(customdb_core PRIVATE NOMINMAX)
endif()
