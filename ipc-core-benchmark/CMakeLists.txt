cmake_minimum_required(VERSION 3.14)
project(ipc_core_benchmark CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# 核心统御引擎源文件库
add_library(ipc_core_static STATIC
    src/substitution_engine.cpp
    src/atp_ctp_engine.cpp
    src/itp_iop_alignment.cpp
)

# 基准测试 1: 交付准确性
add_executable(test_delivery_precision benchmarks/test_delivery_precision.cpp)
target_link_libraries(test_delivery_precision PRIVATE ipc_core_static)

# 基准测试 2: ITP / IOP 协同阻断
add_executable(test_itp_iop_alignment benchmarks/test_itp_iop_alignment.cpp)
target_link_libraries(test_itp_iop_alignment PRIVATE ipc_core_static)

# 基准测试 3: 替代料分配
add_executable(test_substitution_rules benchmarks/test_substitution_rules.cpp)
target_link_libraries(test_substitution_rules PRIVATE ipc_core_static)

# 极限性能压力测试 (200万级)
add_executable(stress_benchmark_2m benchmarks/stress_benchmark_2m.cpp)
target_link_libraries(stress_benchmark_2m PRIVATE ipc_core_static)
