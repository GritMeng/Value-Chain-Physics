cmake_minimum_required(VERSION 3.14)
project(ipc_core_benchmark CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(ipc_core_static STATIC
    src/substitution_engine.cpp
    src/atp_ctp_engine.cpp
    src/itp_iop_alignment.cpp
    src/sandbox_operators.cpp
)

add_executable(test_delivery_precision benchmarks/test_delivery_precision.cpp)
target_link_libraries(test_delivery_precision PRIVATE ipc_core_static)

add_executable(test_substitution_rules benchmarks/test_substitution_rules.cpp)
target_link_libraries(test_substitution_rules PRIVATE ipc_core_static)

add_executable(test_codp_positioning benchmarks/test_codp_positioning.cpp)
target_link_libraries(test_codp_positioning PRIVATE ipc_core_static)

add_executable(test_push_pull_router benchmarks/test_push_pull_router.cpp)
target_link_libraries(test_push_pull_router PRIVATE ipc_core_static)

add_executable(test_itp_iop_alignment benchmarks/test_itp_iop_alignment.cpp)
target_link_libraries(test_itp_iop_alignment PRIVATE ipc_core_static)

add_executable(test_two_stage_iop_network benchmarks/test_two_stage_iop_network.cpp)
target_link_libraries(test_two_stage_iop_network PRIVATE ipc_core_static)

add_executable(stress_benchmark_2m benchmarks/stress_benchmark_2m.cpp)
target_link_libraries(stress_benchmark_2m PRIVATE ipc_core_static)
