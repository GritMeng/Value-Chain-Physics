# IPC Core Engine - 决策质量与极速性能基准开源库 (IPC Core Open Benchmarks)

> **轻量级、可审计、零依赖的 C++17 供应链决策微内核与百万级性能测试集**

[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-green.svg)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-orange.svg)]()

---

> [!IMPORTANT]
> **📢 关于 IPC 系统覆盖范围、专利保护与开源目的的官方声明 (Official System Declaration)**
> 
> 1. **全链路覆盖范围 (End-to-End Governance Coverage)**：
>    IPC (Intelligent Planning & Control) 系统完整覆盖从 **IBP (Integrated Business Planning 业务共识与需求分解)** 到 **ITP (Intelligent Tactical Planning 战术主计划防波堤)**、**IOP (Intelligent Operational Planning 执行计划配额阻断)**，直至 **车间级详细调度与排程 (Shop-Floor Dispatching & Scheduling)** 的端到端供应链协同闭环。
> 
> 2. **为什么暂未 100% 全量开源？(Active Patent Filing Protection)**：
>    由于 IPC 统御引擎中大量突破性的求解架构、多沙箱推演与并发水位算子目前**正处于专利正式申报与法律审查流程中 (Patents Pending)**。为保护核心商业资产与知识产权，系统暂无法将生产环境下的全量商业代码库（包含 DuckDB 物理适配层、大模型预测中枢及完整商业控制塔）100% 无保留公开。
> 
> 3. **开源 `ipc-core-benchmark` 的真正目的 (Proving Physical & Mathematical Feasibility)**：
>    我们之所以开放本开源微内核基准测试集，是为了向全球开发者、架构师与工业同行**证明“百万级规模毫秒级求解”与“交付准且快”在物理与数学上是完全真实可达的 (Physically & Mathematically Achievable)**。本仓库开放脱敏后的 C++17 核心算法算子与 50万需求 / 200万 SKU 基准压测套件，供全球专家直接下载、本地一键运行并物理对账。

---

## 📌 项目定位 (Project Overview)

本仓库是 **IPC 智能计划控制塔 (Intelligent Planning & Control Workspace)** 的开源核心算子与基准测试集。

外部开发者可以**直接 Clone/下载本代码，并在本地一键编译运行**，直观验证 IPC 统御引擎在**决策质量**与**计算速度**上的突破：

```text
               ┌────────────────────────────────────────────────────────┐
               │              IPC Core Engine Architecture              │
               └───────────────────────────┬────────────────────────────┘
                                           │
         ┌─────────────────────────────────┴─────────────────────────────────┐
         ▼                                                                   ▼
┌─────────────────────────────────┐                         ┌─────────────────────────────────┐
│     1. 决策质量 (Quality)       │                         │       2. 决策速度 (Speed)       │
├─────────────────────────────────┤                         ├─────────────────────────────────┤
│ • 交付准且快 (Real-time ATP/CTP)│                         │ • 200万 SKU 级别毫秒级响应      │
│ • 主计划与执行计划协同 (ITP/IOP) │                         │ • SoA 裸金属连续内存调度        │
│ • 三级动态替代料分配法则        │                         │ • 零堆内存分配 (Zero-Heap)      │
└─────────────────────────────────┘                         └─────────────────────────────────┘
```

---

## 🌟 核心开源算子 (Auditable Micro-Kernel Operators)

### 1. 交付又准又快 (Real-Time ATP/CTP Delivery Promising)
* **算子位置**：`include/ipc_core/atp_ctp_engine.h` & `src/atp_ctp_engine.cpp`
* **解法亮点**：支持多层 BOM 深度优先展开与瓶颈工时预留；在能力或物料不足时，触发 **零堆内存回滚 (Zero-Heap Rollback)**，确保微秒级求得精准承诺交期且不占用非法资源。

### 2. 主计划与执行计划协同 (Master Plan vs. Execution Plan Alignment)
* **算子位置**：`include/ipc_core/itp_iop_alignment.h` & `src/itp_iop_alignment.cpp`
* **解法亮点**：ITP 战术主计划生成宏观产能防波堤（Soft Capacity Buffer），IOP 执行计划在车间排产时进行刚性配额阻断（Hard Quota Blocking），彻底解决“计划下发到车间后发生漂移与抢料”的行业难题。

### 3. 三级动态替代料分配 (Material Substitution Engine)
* **算子位置**：`include/ipc_core/substitution_engine.h` & `src/substitution_engine.cpp`
* **解法亮点**：支持一类（按历史配额比例平衡）、二类（组内优先级选优）、三类（跨组动态归一化、批量倍数与安全库存保护）替代料分配决策。

---

## 📊 性能基准看板 (Benchmark Performance)

在 2,000,000 SKU 规模及 500,000 条并发需求下，运行 `bin/stress_benchmark_2m.exe` 的实测数据如下：

| 测试项目 (Benchmark Test) | 数据规模 (Dataset Size) | 全量耗时 (Latency) | 吞吐量 (Throughput) |
| :--- | :--- | :--- | :--- |
| **ITP 防波堤主计划生成** | 500,000 Demands | **~18 ms** | ~27,700,000 / 秒 |
| **IOP 车间协同刚性阻断** | 500,000 Demands | **~24 ms** | ~20,800,000 / 秒 |
| **全链路协同总响应** | **2,000,000 SKUs** | **< 45 ms** | **> 11,000,000 / 秒** |

*(注：测试环境为 AMD Ryzen 9 7950X / 64GB DDR5 / Windows 11，无 GPU 依赖，纯 CPU 裸金属调度)*

---

## 🛠️ 快速开始：本地编译与运行 (Quick Start)

仓库完全零外部第三方库依赖，只需具备标准 C++17 编译器（MSVC / GCC / Clang）或 Python 3。

### 方式 1：Windows 环境一键编译运行 (One-Click Windows Build)

直接双击运行根目录下的批处理脚本：
```cmd
.\compile_and_run.bat
```

### 方式 2：使用 CMake 跨平台编译 (Linux / macOS / Windows)

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release

# 运行四个基准测试套件
./bin/test_delivery_precision
./bin/test_itp_iop_alignment
./bin/test_substitution_rules
./bin/stress_benchmark_2m
```

---

## 🔍 数学正确性审计与交叉校验 (Cross Validation)

为了证明引擎逻辑不仅快，而且**数学上 100% 精确与可被审计**，本仓库提供了 Python 独立数学参考引擎与自动化交叉校验脚本：

```bash
python verifier/cross_validator.py
```

校验器会自动对比 Python 参考数学解法与 C++ 极速统御引擎的输出 JSON，验证替代料配额与 ITP/IOP 配额拦截的 100% 结果一致性。

关于完整数学公式与推导白皮书，请参阅 [docs/MATHEMATICAL_SPEC.md](docs/MATHEMATICAL_SPEC.md)。

---

## 📄 开源协议 (License)

本项目采用 [Apache 2.0 License](LICENSE) 协议开源。
