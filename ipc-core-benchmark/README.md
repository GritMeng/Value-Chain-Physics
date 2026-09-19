# IPC Core Engine — 计算内核与基准测试

> **零依赖 · 可审计 · 可复现的供应链决策微内核 (C++17)**
>
> 本分支仅包含 **代码 + 测试数据 + 本文档**。理论著作、论文手稿等内容不在此分支。

本仓库开放 IPC (Intelligent Planning & Control) 统御引擎中脱敏后的 **4 个核心计算引擎**
与配套的 **CSV 测试数据集**，用于验证「百万级规模毫秒级求解」在物理与数学上的可达性。

---

## 目录

- [一、项目结构](#一项目结构)
- [二、快速开始](#二快速开始)
- [三、4 个核心计算引擎](#三4-个核心计算引擎)
  - [引擎 1：ATP / CTP 交付承诺](#引擎-1atp--ctp-交付承诺引擎)
  - [引擎 2：ITP 主计划防波堤](#引擎-2itp-主计划防波堤引擎)
  - [引擎 3：IOP 执行计划刚性阻断](#引擎-3iop-执行计划刚性阻断引擎)
  - [引擎 4：替代料动态分配](#引擎-4替代料动态分配引擎)
- [四、统一数据契约](#四统一数据契约)
- [五、测试数据](#五测试数据)
- [六、基准测试结果](#六基准测试结果)
- [七、数学正确性对账](#七数学正确性对账)
- [八、构建说明](#八构建说明)

---

## 一、项目结构

```
ipc-core-benchmark/
├── README.md                        # 本文档
├── CMakeLists.txt                   # 跨平台构建
├── build_and_run.sh                 # Linux/macOS 一键编译运行
├── compile_and_run.bat              # Windows/MSVC 一键编译运行
│
├── include/ipc_core/                # 对外接口（头文件即契约）
│   ├── types.h                      # ★ 全部数据结构定义
│   ├── data_loader.h                # CSV 数据加载器
│   ├── atp_ctp_engine.h             # 引擎 1 接口
│   ├── itp_iop_alignment.h          # 引擎 2/3 接口
│   └── substitution_engine.h        # 引擎 4 接口
│
├── src/                             # 引擎实现
│   ├── data_loader.cpp
│   ├── atp_ctp_engine.cpp           # 引擎 1 实现
│   ├── itp_iop_alignment.cpp        # 引擎 2/3 实现
│   └── substitution_engine.cpp      # 引擎 4 实现
│
├── data/                            # ★ 测试数据 (CSV)
│   ├── README.md                    # 数据字典
│   ├── generate_data.py             # 确定性数据生成器
│   ├── parts.csv                    # 2,000,000 行
│   ├── demands_master.csv           # 500,000 行
│   ├── demands_execution.csv        # 500,000 行
│   ├── bom.csv / atp_supply.csv
│   ├── capacity.csv / substitution_group.csv
│   └── sample/                      # 迷你数据集（单步调试）
│
├── benchmarks/                      # 基准测试
│   ├── test_delivery_precision.cpp      # 引擎 1 单元基准
│   ├── test_itp_iop_alignment.cpp       # 引擎 2/3 单元基准
│   ├── test_substitution_rules.cpp      # 引擎 4 单元基准
│   └── stress_benchmark_2m.cpp          # 极限压测（读 CSV）
│
├── verifier/                        # 独立数学对账
│   ├── reference_engine.py          # Python 参考实现
│   └── cross_validator.py           # Python ↔ C++ 交叉校验
│
└── docs/
    └── MATHEMATICAL_SPEC.md         # 数理规范白皮书
```

---

## 二、快速开始

### 1. 生成测试数据

```bash
python3 data/generate_data.py            # 全量数据 (~145 MB)
python3 data/generate_data.py --sample   # 仅迷你集 (~5 KB)
```

### 2. 编译并运行（Linux / macOS）

```bash
./build_and_run.sh
```

### 3. 编译并运行（Windows / MSVC）

```cmd
.\compile_and_run.bat
```

### 4. 使用 CMake（跨平台）

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
./bin/test_delivery_precision
./bin/test_itp_iop_alignment
./bin/test_substitution_rules
./bin/stress_benchmark_2m ../data
```

### 5. 数学交叉对账

```bash
cd verifier && python3 cross_validator.py
```

---

## 三、4 个核心计算引擎

> 4 个引擎共享 `types.h` 中的同一套数据契约，彼此不直接调用，可独立测试与独立替换。

```
                    ┌──────────────────────────────────────────────┐
                    │            统一数据契约 types.h               │
                    │  PartSiteRecord / IndependentDemand /        │
                    │  FlatBomItem / ATPSupplyNode / CapacityRecord │
                    └───────────────────┬──────────────────────────┘
                                        │
        ┌───────────────┬───────────────┼───────────────┬───────────────┐
        ▼               ▼               ▼               ▼
  ┌───────────┐   ┌───────────┐   ┌───────────┐   ┌───────────┐
  │  引擎 1   │   │  引擎 2   │   │  引擎 3   │   │  引擎 4   │
  │ ATP/CTP   │   │   ITP     │   │   IOP     │   │  替代料   │
  │ 交付承诺  │   │ 主计划    │   │ 执行计划  │   │  分配     │
  └───────────┘   └─────┬─────┘   └─────▲─────┘   └───────────┘
                        │  配额下派     │
                        └───────────────┘
```

---

### 引擎 1：ATP / CTP 交付承诺引擎

**职责**：给定一个物料、期望交期与数量，递归展开多层 BOM、预留库存与产能，求出**最快可承诺交付日**。
失败时执行**零堆内存回滚**，保证不留下非法资源占用。

#### 入口函数

```cpp
// include/ipc_core/atp_ctp_engine.h
ATPQueryResult promise_delivery_date(
    uint32_t part_id,                  // 目标物料
    int requested_due_day,             // 期望交期（天）
    double qty,                        // 需求数量
    uint64_t priority,                 // 优先级（越小越高）
    std::vector<std::vector<ATPSupplyNode>>& atp_supplies,   // 供给（按 part_id 索引）
    std::vector<CapacityRecord>& capacity_records,           // 产能（可写，会被扣减）
    const std::vector<PartSiteRecord>& parts,                // 物料主数据
    const std::vector<FlatBomItem>& boms                     // 产品结构
);
```

#### 输入数据

| 输入 | 来源文件 | 关键字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `part_id` | 调用方指定 | — | 目标物料 ID |
| `requested_due_day` | 调用方指定 | — | 期望交期，引擎在 `[d, d+10]` 窗口内搜索 |
| `qty` / `priority` | 调用方指定 | — | 需求数量与优先级 |
| `atp_supplies` | `atp_supply.csv` | `supply_type`、`available_day`、`qty` | 在手/在途/计划产出节点，**会被写回** `allocated_qty` |
| `capacity_records` | `capacity.csv` | `work_center`、`day`、`capacity_hours` | 工作中心产能，**会被写回** `allocated_hours` |
| `parts` | `parts.csv` | `lead_time` | 提前期，决定 BOM 展开的开工日 |
| `boms` | `bom.csv` | `parent_id`、`child_id`、`usage_qty` | 多层 BOM 拓扑 |

#### 输出数据

```cpp
struct ATPQueryResult {
    bool   is_fulfillable;          // 输出: 是否可满足
    int    promised_day;            // 输出: 承诺交付日（失败为 -1）
    double promised_qty;            // 输出: 承诺数量
    double total_capacity_used;     // 输出: 占用总工时
    size_t rollback_steps_count;    // 输出: 回滚次数（>0 表示曾失败重试）
};
```

| 输出 | 含义 |
| :--- | :--- |
| `is_fulfillable` | `true` = 在 `[d, d+10]` 内找到可行交期 |
| `promised_day` | 最早可交付日；`-1` 表示窗口内全部失败 |
| `promised_qty` | 成功时等于输入 `qty` |
| `total_capacity_used` | 本次预留消耗的瓶颈工时合计 |
| `rollback_steps_count` | 每个失败日触发一次回滚计数，用于验证零堆回滚机制生效 |

> **副作用（重要）**：引擎成功时直接修改 `atp_supplies[*].allocated_qty` 与
> `capacity_records[*].allocated_hours`。失败路径会通过 `temp_allocations` /
> `temp_capacity_allocations` 栈式逆序归还，保证状态不被污染。
> 每个查询后如需独立结果，调用方须自行重置这两个字段。

**核心公式**：$t^* = \min\{ t \mid S_{avail}(p,t) + C_{avail}(p, t-L_p) \ge Q \}$

---

### 引擎 2：ITP 主计划防波堤引擎

**职责**：汇总主计划需求，按 `⟨日, 物料族, 客户组, 大区⟩` 四维键生成**带缓冲系数的配额防波堤**，
作为下游 IOP 执行计划的授权上限。

#### 入口函数

```cpp
// include/ipc_core/itp_iop_alignment.h
std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>
generate_itp_master_allotments(
    const std::vector<IndependentDemand>& master_demands,   // 主计划需求
    const std::vector<PartSiteRecord>& parts,               // 预留参数（当前未参与计算）
    double capacity_buffer_factor = 1.1                     // 缓冲系数 γ，默认 +10%
);
```

#### 输入数据

| 输入 | 来源文件 | 说明 |
| :--- | :--- | :--- |
| `master_demands` | `demands_master.csv` | 主计划需求列表；`due_day`/`qty`/`customer_group`/`region` 参与计算 |
| `capacity_buffer_factor` | 调用方指定 | 防波堤缓冲系数 γ ≥ 1.0（压测用 1.15） |
| `parts` | `parts.csv` | ⚠️ 当前接口保留但**未参与计算** |

#### 约束键构造规则

```
Key = ⟨ due_day , part_id / 10 , hash(customer_group) % 100 , hash(region) % 10 ⟩
      └─ 时间 ─┘ └── 物料族 ──┘ └────── 客户组 ────────┘ └─ 大区 ─┘
```

> ⚠️ `part_id / 10` 是占位式的物料族抽象；`std::hash<std::string>` 跨编译器**结果不稳定**，
> 生产环境应替换为确定性哈希（如 FNV-1a）。

#### 输出数据

```cpp
using AllotmentMap =
    std::unordered_map<AllotmentConstraintKey, AllotmentState, AllotmentConstraintKeyHash>;

struct AllotmentState {
    double total_quota;    // 输出: 该键的最大配额（= Σ qty × γ）
    double consumed_qty;   // 输出: 已消耗配额（初始 0，由引擎 3 写入）
};
```

| 输出 | 含义 |
| :--- | :--- |
| map 的 key | 四维配额约束项，数量 = 去重后的 `⟨日, 族, 客户组, 大区⟩` 组合数 |
| `total_quota` | 该组合的最大授权量，$= \gamma \cdot \sum_{i \in Key} Q_i$ |
| `consumed_qty` | 由引擎 3 在执行阶段累加写入，本引擎输出时恒为 0 |

**核心公式**：$\mathcal{A}(Key) = \gamma \cdot \sum_{i \in \text{MasterDemands}(Key)} Q_i$

---

### 引擎 3：IOP 执行计划刚性阻断引擎

**职责**：在引擎 2 给出的刚性配额下，按优先级排序逐单校验执行需求；
**超出配额上限的订单被强制拦截阻断**，防止车间越权抢料导致计划漂移。

#### 入口函数

```cpp
IOPExecutionResult run_iop_execution_alignment(
    const std::vector<IndependentDemand>& execution_demands,   // 执行需求
    const std::vector<PartSiteRecord>& parts,                  // 预留参数（当前未参与计算）
    AllotmentMap& allotment_constraints                        // ← 引擎 2 输出，会被就地扣减
);
```

#### 输入数据

| 输入 | 来源文件 | 说明 |
| :--- | :--- | :--- |
| `execution_demands` | `demands_execution.csv` | 执行需求；按 `priority` 升序（小值优先）处理 |
| `allotment_constraints` | **引擎 2 输出** | 就地修改 `consumed_qty`，是引擎 2→3 的数据纽带 |
| `parts` | `parts.csv` | ⚠️ 当前接口保留但**未参与计算** |

#### 输出数据

```cpp
struct IOPExecutionResult {
    uint32_t total_orders;         // 输出: 总请求单数
    uint32_t scheduled_orders;     // 输出: 成功下派单数
    uint32_t blocked_orders;       // 输出: 被配额拦截单数
    double   total_fulfilled_qty;  // 输出: 成功下派需求量合计
    double   quota_utilization;    // 输出: 配额消耗率 = Σconsumed / Σquota
};
```

| 输出 | 含义 |
| :--- | :--- |
| `total_orders` | 输入执行需求条数 |
| `scheduled_orders` | 配额校验通过并扣减的单数 |
| `blocked_orders` | 超配额 / 无授权被拦截的单数 |
| `total_fulfilled_qty` | 成功下派的需求量总和 |
| `quota_utilization` | 全局配额使用率，`1.0` = 100% |

**约束不等式**：$\sum_{j \le k} Q_j^{IOP} \le \mathcal{A}(Key)$，突破即硬阻断。

> **副作用（重要）**：引擎会**就地累加** `allotment_constraints[*].consumed_qty`。
> 重复运行前须重新调用引擎 2 生成新配额，或用 `demands_master.csv` 重建。

---

### 引擎 4：替代料动态分配引擎

**职责**：当主料不足时，按三类替代规则在替代料组内选优并消纳库存。

#### 入口函数（3 个算子）

```cpp
// include/ipc_core/substitution_engine.h

// 一类：历史配额比例平衡（纠偏式平摊）
uint32_t allocate_class1(
    double net_demand,                        // 净需求
    std::vector<FlatBomItem*>& group_items,   // 替代料组成员（可写 historical_qty）
    std::vector<double>& current_on_hand      // ⚠️ 当前未使用
);

// 二类：组内固定优先级选优
uint32_t allocate_class2(
    std::vector<FlatBomItem*>& group_items
);

// 三类：跨组动态归一化 + 水位消纳
void allocate_class3(
    double net_demand,
    std::vector<FlatBomItem*>& group_items,
    std::vector<double>& current_on_hand,          // 可写: 扣减库存
    int day,
    uint32_t part_id,
    std::vector<AlternateAllocationRecord>& out_alt_records,  // 输出: 分配明细
    const std::vector<PartSiteRecord>& parts       // 读取 safety_stock
);
```

#### 输入数据

| 输入 | 来源文件 | 关键字段 | 说明 |
| :--- | :--- | :--- | :--- |
| `net_demand` | 调用方指定 | — | 待消纳的净需求 |
| `group_items` | `bom.csv` / `substitution_group.csv` | `alt_class`、`target_ratio`、`historical_qty`、`lot_size` | 替代料组，**`historical_qty` 会被累加** |
| `current_on_hand` | `atp_supply.csv`（On-Hand） | — | 当前在手库存**向量**，按 `part_id` 索引，Class 3 会扣减 |
| `parts` | `parts.csv` | `safety_stock` | 安全库存保护带，Class 3 不可侵占 |

#### 输出数据

| 输出 | 形式 | 含义 |
| :--- | :--- | :--- |
| Class 1 / 2 | `uint32_t` | 选中的替代料 `child_id`；无解返回 `(uint32_t)-1` |
| Class 3 | `void` + 出参 `out_alt_records` | 分配明细记录列表 |

```cpp
struct AlternateAllocationRecord {
    uint32_t day;              // 分配日
    uint32_t parent_part_id;   // 父件（需求物料）
    uint32_t alt_part_id;      // 实际分配的替代料 ID
    double   allocated_qty;    // 实际分配量（已按 Lot 向上对齐）
    int      day_allocated;    // 分配发生日
    uint8_t  alt_class;        // 替代等级（固定为 3）
};
```

#### 三类算法判据

| 等级 | 策略 | 选优公式 | 输出 |
| :--- | :--- | :--- | :--- |
| **Class 1** | 偏差极小化比例分流 | $\arg\max_i \lvert Q_i^{hist} - Q_{total}\theta_i \rvert$ | 选中 ID |
| **Class 2** | 组内固定优先级 | $\arg\min_i \; Q_i^{hist}/\max(\theta_i,\epsilon)$ | 选中 ID |
| **Class 3** | 跨组动态归一化 + 水位消纳 | 每轮重算 $\theta_i^{cur} = Q_i^{due}/\sum Q_j^{due}$ | 明细记录 |

Class 3 消纳量约束：
$$Q_i^{actual} = \left\lceil \frac{\theta_i^{cur} \cdot Q_{net}}{\text{Lot}_i} \right\rceil \cdot \text{Lot}_i, \qquad
Q_i^{consumed} = \min\left(Q_i^{actual},\ \max(0, \text{OnHand}_i - \text{SS}_i),\ Q_{net}\right)$$

> ⚠️ `allocate_class1` 的 `current_on_hand` 参数**当前未被使用**，
> 即一类替代料选优暂未校验库存可行性，存在选中缺料替代料的潜在风险。

---

## 四、统一数据契约

全部结构定义于 `include/ipc_core/types.h`（纯 POD，无虚函数、无指针追踪）。

| 结构体 | 用途 | 对应 CSV |
| :--- | :--- | :--- |
| `PartSiteRecord` | 物料-站点主数据 | `parts.csv` |
| `IndependentDemand` | 独立需求 | `demands_master.csv` / `demands_execution.csv` |
| `FlatBomItem` | 扁平 BOM + 替代料属性 | `bom.csv` |
| `CapacityRecord` | 工作中心产能 | `capacity.csv` |
| `ATPSupplyNode` | 供给节点 | `atp_supply.csv` |
| `AllotmentConstraintKey` / `AllotmentState` | 配额键与状态 | 引擎 2 内部生成 |
| `AlternateAllocationRecord` | 替代分配记录 | 引擎 4 输出 |

字段级说明见 [`data/README.md`](data/README.md)。

---

## 五、测试数据

全部数据为 **CSV 明文**，可查看、可修改、可替换。由 `data/generate_data.py`
**确定性生成**（纯函数推导、无随机），每次生成结果完全一致。

| 文件 | 行数 | 大小 | 用途 |
| :--- | ---: | ---: | :--- |
| `parts.csv` | 2,000,001 | 96 MB | 物料-站点主数据 |
| `demands_master.csv` | 500,001 | 22 MB | ITP 主计划需求 |
| `demands_execution.csv` | 500,001 | 22 MB | IOP 执行需求（含 1.3× 超量插单） |
| `bom.csv` | 9 | <1 KB | 产品结构 + 四类替代料 |
| `atp_supply.csv` | 13 | <1 KB | 供给节点 |
| `capacity.csv` | 61 | <1 KB | 工作中心产能 |
| `substitution_group.csv` | 7 | <1 KB | 替代料组定义 |
| `sample/*` | 迷你 | <5 KB | 单步调试与格式示例 |

> **`demands_execution.csv` 为何与主计划不同？**
> 原实现中主计划与执行计划共用同一份数据，配额永远用不完（`blocked_orders` 恒为 0），
> 阻断路径完全未被覆盖。本数据集刻意让部分执行需求放大 1.3 倍，使其突破
> `50 × 1.15 = 57.5` 的防波堤配额，从而**真实覆盖刚性阻断分支**。

---

## 六、基准测试结果

实测环境：macOS / Apple clang 21.0.0 / `-O2`

### 极限压测（2,000,000 SKU · 500,000 需求）

```
[Phase 1] CSV 加载 ............ 917.78 ms   (2,000,000 SKU + 1,000,000 需求)
[Phase 2] ITP 主计划 (500k) ... 18.94 ms    → 26,398,981 Demands/Sec
[Phase 3] IOP 执行计划 (500k) . 29.74 ms    → 16,811,196 Orders/Sec
────────────────────────────────────────────────────────────────
求解总延迟 .................... 48.68 ms
求解吞吐量 .................... 20,541,385 需求/秒
IOP 结果: 下派 300,000 / 阻断 200,000（阻断路径已覆盖）
```

### 三个单元基准

| 基准 | 场景 | 结果 |
| :--- | :--- | :--- |
| `test_delivery_precision` | 30 件 → 承诺 Day 3；100 件 → 阻断并回滚 11 步 | ✅ PASS |
| `test_itp_iop_alignment` | 4 单（含 1 单越权插单）→ 下派 3 / 拦截 1 | ✅ PASS |
| `test_substitution_rules` | Class1 纠偏选中 ID 1；Class3 分配 ⌈6/5⌉×5 = 10，安全库存 10 未被侵占 | ✅ PASS |

> 数据加载（918 ms）为一次性 I/O 开销，不计入求解延迟；求解本身为纯内存计算。

---

## 七、数学正确性对账

`verifier/` 提供独立的 **Python 参考实现**，与 C++ 引擎逐案对账，验证数学逻辑 100% 一致：

```bash
cd verifier && python3 cross_validator.py
```

```
[Validation 1] 一类替代料平摊配额   Python 选优=1  ↔  C++ 期望=1      → PASSED
[Validation 2] ITP/IOP 配额阻断     Python 3/1     ↔  C++ 期望 3/1    → PASSED
```

完整数学推导见 [`docs/MATHEMATICAL_SPEC.md`](docs/MATHEMATICAL_SPEC.md)。

---

## 八、构建说明

| 项目 | 要求 |
| :--- | :--- |
| 编译器 | 任意支持 C++17 的编译器（MSVC / GCC / Clang） |
| 第三方依赖 | **无**（零外部库，仅标准库） |
| Python（可选） | 3.7+（用于数据生成与交叉校验） |
| CMake（可选） | 3.14+（亦可直接用脚本编译） |

**已验证平台**：macOS (Apple clang 21)、Windows (MSVC 2017+)、Linux (GCC 9+)

---

## 九、开源协议

本项目采用 [Apache 2.0 License](LICENSE) 协议开源。

> **关于系统覆盖范围**：IPC 系统完整覆盖 IBP → ITP → IOP → 车间排程的端到端闭环。
> 本仓库开放的 4 个计算引擎为脱敏后的核心算子与基准测试集，用于证明
> 「百万级规模毫秒级求解」在物理与数学上可达。生产环境全量商业代码库
> （DuckDB 物理适配层、大模型预测中枢、完整商业控制塔）因**专利申报中**暂未开源。
