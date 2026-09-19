cmake_minimum_required(VERSION 3.14)
project(ipc_core_benchmark CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(include)

# 核心引擎静态库 (4 个计算引擎 + 数据加载器)
add_library(ipc_core_static STATIC
    src/data_loader.cpp
    src/atp_ctp_engine.cpp            # 引擎 1: ATP/CTP 交付承诺
    src/itp_iop_alignment.cpp         # 引擎 2/3: ITP 主计划 + IOP 执行计划
    src/substitution_engine.cpp       # 引擎 4: 替代料分配
)

# 基准 1: ATP/CTP 交付精度与零堆回滚
add_executable(test_delivery_precision benchmarks/test_delivery_precision.cpp)
target_link_libraries(test_delivery_precision PRIVATE ipc_core_static)

# 基准 2: ITP/IOP 协同阻断
add_executable(test_itp_iop_alignment benchmarks/test_itp_iop_alignment.cpp)
target_link_libraries(test_itp_iop_alignment PRIVATE ipc_core_static)

# 基准 3: 替代料一等/二等/三等规则
add_executable(test_substitution_rules benchmarks/test_substitution_rules.cpp)
target_link_libraries(test_substitution_rules PRIVATE ipc_core_static)

# 基准 4: 极限性能压测 (从 data/*.csv 加载)
add_executable(stress_benchmark_2m benchmarks/stress_benchmark_2m.cpp)
target_link_libraries(stress_benchmark_2m PRIVATE ipc_core_static)

# 让压测程序默认以项目根为工作目录，便于找 data/
set_target_properties(stress_benchmark_2m PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# CLI: CSV 输入 / CSV 输出 命令行驱动 (供 Node.js 编排层调用)
# 说明: 复用 ipc_core_static 中既有算子, 不修改既有引擎与基准。
add_executable(ipc_engine_cli cli/ipc_engine_cli.cpp)
target_include_directories(ipc_engine_cli PRIVATE cli)
target_link_libraries(ipc_engine_cli PRIVATE ipc_core_static)
set_target_properties(ipc_engine_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
