# IPC 智能计划求解器内核：系统演进白皮书与技术规范说明书

本白皮书详细阐述了 **智能计划与控制 (Intelligent Planning & Control, IPC)** 求解器引擎从最初的单体研究性代码，演进并重构成世界级裸金属计算引擎的完整技术历程。白皮书涵盖了系统级演进、模块化架构剖析、底层数学与算法代数式推导，以及日常编译、测试与维护的技术规范。

---

## 目录
- [一、 系统演进历程：原始概念模型 vs 世界级架构设计](#一-系统演进历程原始概念模型-vs-世界级架构设计)
- [二、 核心模块结构与 API 规范说明](#二-核心模块结构与-api-规范说明)
- [三、 底层数学原理与算法代数式推导](#三-底层数学原理与算法代数式推导)
- [四、 编译、测试与部署维护指南](#四-编译测试与部署维护指南)

---

## 一、 系统演进历程：原始概念模型 vs 世界级架构设计

在 IPC 内核的演进中，我们经历了从“功能实现”向“高性能、高维护性世界级求解器”的跨越。下表对比了系统重构前后的核心技术特征：

| 演进维度 | 原始概念模型 (Initial Codebase) | 进化后的世界级架构 (Current Architecture) |
| :--- | :--- | :--- |
| **模块化与解耦** | 超过 35,000 行的单体文件 `src/engine_main.cpp`，数据库存取、核心运筹、测试数据生成混合在一起。 | 11个逻辑模块彻底解耦，公共接口、核心求解、数据适配和测试套件完全物理分离。 |
| **命名空间卫生** | 全局命名空间污染。变量、结构体和控制函数直接声明在全局，极易发生符号冲突。 | 全系统完全封闭在 `namespace ipc::` 中。各模块相互独立引用，消除了全局变量和符号重定义风险。 |
| **编译机制** | 采用宏定义开关（如 `#ifndef STRESS_TEST_BUILD`）来强行切分测试和生产入口，依赖关系混乱。 | 零宏定义黑客手段。核心算法编译为独立的对象单元，测试套件和主运行程序作为独立的编译单元进行链接。 |
| **数据库适配** | 核心 MRP 与 DBD 求解函数中直接嵌入了 DuckDB 的 SQL 语句和连接指针，强耦合。 | 引入 **适配器模式 (Adapter Pattern)**，构建了 `ipc::DbAdapter`。算法层只与扁平的 C++ 内存数据对象交互，数据库实现对核心算法透明。 |
| **内存与并发安全** | 存在未加锁的多线程读写，时序分配上存在严重的抢占和脏数据隐患。 | 引入了基于 CAS (Compare-And-Swap) 无锁乐观库存池，并通过线程局部的暂存追踪栈实现 $O(1)$ 的零堆分配事务回滚。 |

---

## 二、 核心模块结构与 API 规范说明

经过重构，IPC 内核的源文件和头文件被精细划分到统一的目录树中：

```
h:/IPC/
├── include/                   # 公共接口头文件
│   ├── ipc_types.h            # 全局实体结构定义 (受 namespace ipc 包裹)
│   └── ipc/                   # 模块化公共接口
│       ├── database.h         # 数据库适配器 (ipc::DbAdapter)
│       ├── dbd_engine.h       # 有限产能与时序 ATP 引擎
│       ├── mrp_engine.h       # LBL MRP 核心消纳引擎
│       ├── coproduct.h        # 联副产品分级降级引擎
│       ├── vocab.h            # 字符串映射词汇表 (ipc::PartVocab)
│       ├── globals.h          # 全局运行时上下文
│       ├── math_utils.h       # 统计学与 Holt-Winters 数学工具
│       ├── lsc_tree.h         # 级联物料清单树
│       ├── substitution.h     # 替代料专利算法
│       └── tests.h            # 测试与 Benchmark 生成器
└── src/                       # 核心 C++ 实现代码
```

### 核心接口 API 规范

#### 1. 数据库适配器 `ipc::DbAdapter`
定义于 [database.h](file:///h:/IPC/include/ipc/database.h)，负责执行 DuckDB 表的事务读写，并负责内存 SoA 结构的初始化加载：
```cpp
namespace ipc {
    class DbAdapter {
    public:
        explicit DbAdapter(duckdb::Connection& con);
        
        // 主数据与需求加载
        void load_parts(std::vector<PartSiteRecord>& parts);
        void load_boms(std::vector<FlatBomItem>& boms);
        void load_demands(std::vector<IndependentDemand>& demands);
        
        // 计划结果流式回写
        void save_planned_orders(const std::vector<PlannedOrder>& orders);
        void save_dispatch_ledger(const std::vector<PlannedOrderSplit>& splits);
        void save_swap_results(const std::vector<SwapRecord>& swaps);
    private:
        duckdb::Connection& connection;
    };
}
```

#### 2. 无级延时时序有限产能调度 API
定义于 [dbd_engine.h](file:///h:/IPC/include/ipc/dbd_engine.h)，通过递归回溯探针决策最优交付时间（CTP）：
```cpp
namespace ipc {
    bool reserve_atp_and_capacity_recursive(
        uint32_t part_id,
        int due_day,
        double qty,
        uint64_t priority,
        double dimension_val,
        std::vector<std::vector<OTPSupplyNode>>& otp_supplies,
        std::vector<ConstraintRecord>& shared_constraints,
        const std::vector<PartSiteRecord>& parts,
        const std::vector<FlatBomItem>& boms,
        const std::vector<std::vector<size_t>>& local_parent_to_bom,
        const std::vector<SourceConstraintRecord>& source_constraints,
        
        // 事务暂存栈：支持零堆分配高速回滚
        std::vector<OTPSupplyNode*>& temp_allocations,
        std::vector<double>& temp_alloc_qty,
        std::vector<std::pair<size_t, std::pair<int, double>>>& temp_capacity_allocations,
        const std::string& preference_mode,
        std::vector<double>& bom_ltb_consumed,
        std::vector<std::pair<size_t, double>>& temp_ltb_allocations,
        std::unordered_map<uint32_t, int>& active_mix_groups,
        const std::vector<std::vector<double>>& last_dim_val,
        double& out_routing_cost,
        std::vector<PlannedOrderSplit>& temp_po_splits,
        bool is_recursive_child = false
    );
}
```

---

## 三、 底层数学原理与算法代数式推导

IPC 求解器之所以能同时具备高吞吐量与极强的业务控制精度，得益于底层严密的代数模型和物理规律抽象。

### 3.1 时序可用性水位消纳几何算子（双端前缀和 Netting）
传统的冲减计算依赖大量的 `if-else` 分支判断，在 CPU 级流水线（Super-scalar Pipeline）中极易导致分支预测失败。IPC 将其抽象为一维数轴区间交集覆盖。

设对于物料 $P$ 的供应总量上限为 $CS = OnHand + ScheduledReceipts$（现有量与已确认在途量之和）。
沿着天粒度时间轴排好序的需求事件序列为 $ev_0, ev_1, \dots, ev_N$。每个事件 $ev_j$ 的需求数量为 $qty_j$。
定义当前事件 $ev_j$ 在数轴上的**前置累积需求前缀和区间** $[CD_0, CD_1]$，计算公式为：
$$ CD_0 = \sum_{i=0}^{j-1} qty_i \quad \text{和} \quad CD_1 = CD_0 + qty_j $$

#### 消纳算子推导
* **已消纳分配量 (Allocated Quantity)**:
  $$ consumed_j = \max \Big( 0.0, \min(CD_1, CS) - CD_0 \Big) $$
* **传递给下级的净需求缺口 (Net Demand)**:
  $$ net\_demand_j = \max \Big( 0.0, CD_1 - \max(CD_0, CS) \Big) $$

这两条代数式在 C++ 代码中没有任何条件分支逻辑，现代编译器可对其执行高级 **SIMD 向量化并行编译**，消除了分支预测开销。

### 3.2 替代料决策算法与 Lot-Sizing 动态归一化

#### 3.2.1 一类替换：动态配额绝对温差最大优先 (Quota Balancing)
设替代料池 $\mathcal{G}$ 内各零件历史累计消耗量为 $H_{i}$，配额比率为 $R_{i}$。总虚拟分配需求为 $Total = net\_demand + \sum_{i \in \mathcal{G}} H_i$。理论配额量 $Due_{i} = Total \times R_{i}$。
$$ Choice = \arg\max_{i \in \mathcal{G}} |H_{i} - Due_{i}| \quad (\text{平局选 } R_{i} \text{ 最大者}) $$

#### 3.2.2 二类替换：供应商稳定配额评级最低优先 (Low-Rating Priority)
计算供应商消耗配额评级：
$$ Rating_{i} = \frac{H_{i}}{\max(10^{-9}, R_{i})} $$
$$ Choice = \arg\min_{i \in \mathcal{G}} Rating_{i} \quad (\text{平局选 } R_{i} \text{ 最大者}) $$

#### 3.2.3 三类替换：带有 Lot-Size 限制的多轮归一化分配
迭代计算各活跃零件应分配量 $Due_{i} = W_{i} \times rem\_net$。对 $Due$ 最大零件的分配量向上取整：
$$ Actual\_Qty = \lceil \frac{Due_{chosen}}{Lot_{chosen}} \rceil \times Lot_{chosen} $$
消纳相应库存后，将该零件移出活跃集，对剩下的零件配额进行**动态重新归一化**更新：
$$ W_{cand\_next} = \frac{Due_{cand}}{\sum_{j \in \mathcal{A}} Due_{j}} $$

### 3.3 维度匹配与降级使用关系矩阵
当在制半成品（如 Wafer）的测试良率和分级特征（如容量 512MB vs 256MB）与成品订单需求不对称时，系统利用关系代数矩阵进行约束过滤。
设成品需求的规格数值为 $Dim_{req}$，实物库存的规格数值为 $Dim_{inv}$，关系算子为 $\Omega$：
$$ \Omega \in \{ \text{EQ (精确对等)}, \text{GE (降级消纳)}, \text{NE (屏蔽隔离)}, \text{PASS (不受限制)} \} $$
$$ Match(Dim_{inv}, \Omega, Dim_{req}) = \begin{cases} 
      True & \text{若 } \Omega = \text{EQ} \text{ 且 } Dim_{inv} = Dim_{req} \\
      True & \text{若 } \Omega = \text{GE} \text{ 且 } Dim_{inv} \ge Dim_{req} \\
      True & \text{若 } \Omega = \text{NE} \text{ 且 } Dim_{inv} \ne Dim_{req} \\
      True & \text{若 } \Omega = \text{PASS} \\
      False & \text{其他情况}
   \end{cases} $$
通过 GE 算子，引擎实现了以高等级芯片自动替代低等级芯片的降级消纳，充分平抑了晶圆制程波动。

### 3.4 时序有限能力 ATP 与零堆分配事务回滚栈
在时序有限能力排产阶段，订单的加工提前期拉伸计算如下：
$$ start\_day = finish\_day - \lceil LeadTime_{std} + Qty \times RunRate \rceil $$
如果关键资源的维度特征 $last\_dim\_val$ 在该排程日上与当前订单的 $dimension\_val$ 一致，则豁免 setup 时间：
$$ SetupTime = \begin{cases} 0.0 & \text{若 } last\_dim\_val[Resource\_ID][start\_day] == dimension\_val \\ before\_fixed\_factor & \text{否则} \end{cases} $$

#### 零堆分配事务回滚栈原理
为杜绝动态内存分配（Heap Allocation）导致的延迟不确定性，所有 ATP 占用的库存和产能直接在物理数组上进行原位（In-place）修改。
在递归搜索的每一步中，所有的修改记录（对象指针、变动数量、修改天数）均被压入一个线程局部的暂存栈。
* **Commit 成功**：直接将暂存栈大小清零，原位改写生效。
* **Rollback 回滚**：在 $O(1)$ 时间内逆向遍历暂存栈，对修改数据进行还原：
  $$ OnHand[node] \leftarrow OnHand[node] + alloc\_qty $$
  $$ AllocatedCapacity[res][day] \leftarrow AllocatedCapacity[res][day] - alloc\_load $$
  这彻底规避了垃圾回收（GC）和频繁内存申请的开销。

### 3.5 ETO task CPM (关键路径法) 与日历工作日拉伸
对含有研发/定制生产（ETO）特点的复杂工程项目，系统在求解物料需求之前，先进行有向无环图（DAG）的关键路径法 (CPM) 运算。

设任务 $i$ 的工期为 $D_i$，其紧前任务集合为 $Pred_i$，紧后任务集合为 $Succ_i$。
* **最早开工与最早完工时间 (Forward Pass)**:
  $$ ES_i = \max_{j \in Pred_i} (EF_j + Lag_{j, i}) \quad (\text{若 } Pred_i = \emptyset, \text{则 } ES_i = 0) $$
  $$ EF_i = ES_i + D_i $$
* **最迟完工与最迟开工时间 (Backward Pass)**:
  $$ LF_i = \min_{k \in Succ_i} (LS_k - Lag_{i, k}) \quad (\text{若 } Succ_i = \emptyset, \text{则 } LF_i = Due\_Day) $$
  $$ LS_i = LF_i - D_i $$
* **总时差 (Total Float)**:
  $$ TF_i = LS_i - ES_i = LF_i - EF_i $$
* **关键路径定义**:
  若 $TF_i = 0$，则任务 $i$ 处于关键路径上。任何关键路径上的任务延迟都将直接导致最终交付期顺延。

在计算 $ES_i$ 和 $EF_i$ 时，若跨越日历非工作日，系统将自动进行提前期拉伸：
$$ Days\_To\_Skip = \sum_{d \in [start, end]} (1 - IsWorkingDay(Calendar, d)) $$
$$ Actual\_Days\_Span = D_i + Days\_To\_Skip $$

---

## 四、 编译、测试与部署维护指南

Evolution 重构后，IPC 引擎的运行和编译管理逻辑已实现了脚本级自动化。

### 4.1 代码编译规范

* **主求解器引擎编译 (`main_mem3.exe`)**：
  直接在终端运行批处理脚本：
  ```cmd
  scratch\compile_main.bat
  ```
  该批处理会自动检测并载入 MSVC X64 的编译环境（`vcvars64.bat`），调用编译器：
  ```cmd
  cl.exe /EHsc /O2 /openmp /std:c++20 /utf-8 src/*.cpp tests/test_stress.cpp /I include /I IPC\include /Fobuild\ /Fe:main_mem3.exe /link /LIBPATH:IPC\lib duckdb.lib
  ```
  编译生成的 `.obj` 文件会被全部集中输出至 [build/](file:///h:/IPC/build) 文件夹，避免了根目录的混乱。
  
* **独立验证测试器编译 (`test_runner_validation.exe`)**：
  在终端中运行：
  ```cmd
  scratch\compile_validation.bat
  ```
  同样会将 `.obj` 输出定向至 `build/`。

### 4.2 三层对账回归测试套件运行规范

我们建立并保留了三级回归测试防线：

#### 1. 全息单元测试级（Holographic Unit Tests）
* **命令**：
  ```cmd
  .\run_holo_stress.bat
  ```
* **功能**：编译并拉起 `bin/holo_stress.exe`，执行覆盖三层结构的 **46 项全息单元测试**。这包含：
  - PartVocab 哈希去碰撞测试。
  - 64位复合二进制优先级排序测试。
  - 双端前缀和消纳几何算子数学验证。
  - 一、二、三类替代料配额收敛与重归一测试。
  - 事务回滚栈在失败路径下的状态无损还原测试。

#### 2. E2E 多模式集成测试级（E2E Python Integration Tests）
* **命令**：
  ```cmd
  python tests/test_modes_e2e.py
  ```
* **功能**：自动建立测试沙箱数据库 `build/sandbox_modes_test.db`，模拟真实 DuckDB 表下的：
  - **ITP 战术模式**：检测未约束 MRP 与有限产能 DBD 下的自动瓶颈剪裁（Gating Constraint）。
  - **IOP 运营模式**：验证 Allotment 配额防波堤对于 VVIP 和普通订单的硬性拦截和抢占保护。
  - **IBP 预测分解**：验证 Holt-Winters 概率带需求分摊与安全库存（Safety Stock）的流式生成。

#### 3. 200W 超大型性能测试级（200W Scale Performance Stress Tests）
* **命令**：
  ```cmd
  .\run_itp_iop_stress.bat
  ```
* **功能**：编译并在单机上使用超大型虚拟供应链模型进行性能极限压榨。
  - **规模**：200万零件（SKU）、245万 BOM 关系链、50万笔独立需求订单。
  - **验证**：执行包含 **75 项复杂测试指标** 的综合大考，包括跨站点调拨前置期拉伸、多约束联合选线决策、多级安全库存伝导、Swap 呆滞料置换率。
  - **指标要求**：全拓扑消纳计算耗时需控制在毫秒/秒级，OTIF 必须保持在 99% 以上。

---

## 五、 总结

通过本次世界级的架构升级，IPC 内核由一个“研究性单体结构”进化为高度模块化、极致缓存友好、且完全与数据库层级解耦的**企业级高性能求解器引擎**。极简的代码风格与强命名空间保护，为未来的跨部门协同开发（如集成 MetaERP 等）奠定了坚实的基础。
