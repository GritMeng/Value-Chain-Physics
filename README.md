# IPC Core Engine — 供应链决策计算内核

> **零依赖 · 可审计 · 可复现的 C++17 供应链决策内核：4 个核心计算引擎，百万级规模、毫秒级求解。**

本仓库的核心是 **[`ipc-core-benchmark/`](ipc-core-benchmark/README.md) —— 计算引擎本身**：
脱敏后的 4 个 IPC (Intelligent Planning & Control) 核心决策算子、统一数据契约、CSV 测试数据集、
极限压测与数学对账。它回答的问题是：**这些业务场景能不能算准、能算多快、结论能不能被独立复核。**

[`ipc-demo/`](ipc-demo/README.md) 是为引擎服务的**功能实验工具**，不是项目主体。
它把「准备输入 CSV → 调引擎 → 读输出 CSV → 入库 → 出图」编排起来，
让业务使用者**看得见**引擎的输入、决策依据与结论，从而用自己的数据动手实验、理解功能边界。
引擎本身不依赖 demo；demo 只是引擎的一个消费方。

| 目录 | 内容 | 角色 |
| :--- | :--- | :--- |
| [`ipc-core-benchmark/`](ipc-core-benchmark/README.md) | C++17 计算内核（4 个引擎）+ CSV 测试数据 + 跨平台基准 + 数学对账 + CSV 驱动 CLI | **核心：唯一计算源** |
| [`ipc-demo/`](ipc-demo/README.md) | Node.js 编排 + DuckDB + 可视化前端 | **辅助：功能实验与理解工具** |

---

## 一、业务场景覆盖

IPC 覆盖「主计划 → 执行计划 → 车间投料」的供应链计划闭环中的关键决策点。
本仓库开放其中 **4 个核心算子**（脱敏），它们共享同一套数据契约、彼此不直接调用、可独立测试与独立替换。

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

### 场景总览

| # | 引擎 | 业务问题 | 输入 | 输出结论 |
| :--- | :--- | :--- | :--- | :--- |
| 1 | **ATP / CTP 交付承诺** | 给定物料、期望交期与数量，最快能承诺哪天？ | `parts` `bom` `atp_supply` `capacity` | 承诺日、承诺量、预留工时、回滚步数 |
| 2 | **ITP 主计划防波堤** | 主计划需求汇总后，每个四维约束键的授权上限是多少？ | `parts` `demands_master` | 四维配额（`day × family × cust_group × region`） |
| 3 | **IOP 执行计划刚性阻断** | 执行需求按优先级逐单校验，哪些单被配额拦截？ | `parts` `demands_master` `demands_execution` | 下派/阻断单数、满足量、配额利用率 |
| 4 | **替代料动态分配** | 主料不足时，替代料组内该选谁、分多少？ | `parts` `bom` `substitution_group` | 选中替代料、分配明细、安全库存水位 |

### 引擎 1 · ATP / CTP 交付承诺

**场景**：客户或销售问「这个物料，这个数量，最快哪天能交？」。需要在多层 BOM 上递归展开，
同时受库存/在途/计划供给与工作中心产能双边约束，给出**最早可承诺交付日**；失败时执行
**零堆内存回滚**，保证不留下非法资源占用（避免一次失败的试算污染后续查询）。

- 入口函数：`promise_delivery_date(part_id, requested_due_day, qty, priority, atp_supplies, capacity_records, parts, boms)`
- 搜索窗口：`[d, d+10]`，逐候选日试算，不满足即回滚并前进一天
- 核心公式：$t^* = \min\{ t \mid S_{avail}(p,t) + C_{avail}(p, t-L_p) \ge Q \}$
- 关键判据：BOM 展开用量 × 批量、`lead_time` 决定开工日、供给节点 `allocated_qty` 与产能 `allocated_hours` 的可用余量

> 引擎成功时会**就地写回** `atp_supplies[*].allocated_qty` 与 `capacity_records[*].allocated_hours`；
> 失败路径通过栈式逆序归还 `temp_allocations`，保证状态不被污染。

### 引擎 2 · ITP 主计划防波堤

**场景**：主计划（S&OP / IBP 输出）汇总后，需要给下游执行一个**授权上限**，
防止车间越权抢料导致计划漂移。按 `⟨日, 物料族, 客户组, 大区⟩` 四维键聚合主计划需求，
乘上缓冲系数 γ 生成配额防波堤。

- 入口函数：`generate_itp_master_allotments(master_demands, parts, capacity_buffer_factor = 1.1)`
- 约束键：`Key = ⟨ due_day, part_id/10, hash(customer_group) % 100, hash(region) % 10 ⟩`
- 核心公式：$\mathcal{A}(Key) = \gamma \cdot \sum_{i \in \text{MasterDemands}(Key)} Q_i$

### 引擎 3 · IOP 执行计划刚性阻断

**场景**：执行需求（插单、临时单）按优先级逐单校验，**突破主计划配额上限的订单被强制阻断**。
这是「防波堤」真正生效的地方：配额不是软提醒，而是刚性下派闸门。

- 入口函数：`run_iop_execution_alignment(execution_demands, parts, allotment_constraints)`
- 处理顺序：按 `priority` 升序（数值越小优先级越高）
- 约束不等式：$\sum_{j \le k} Q_j^{IOP} \le \mathcal{A}(Key)$，突破即硬阻断
- 输出：总单数 / 下派单数 / 阻断单数 / 满足量 / 配额利用率

> 引擎会**就地累加** `allotment_constraints[*].consumed_qty`（引擎 2 → 引擎 3 的数据纽带）。
> 重复运行前须重新调用引擎 2 生成新配额。

### 引擎 4 · 替代料动态分配

**场景**：主料不足时，按替代等级在组内选优并消纳库存，同时**不侵占安全库存**。
三个算子分别对应三类替代策略：

| 等级 | 策略 | 选优公式 | 输出 |
| :--- | :--- | :--- | :--- |
| **Class 1** | 历史配额比例平衡（纠偏式平摊） | $\arg\max_i \lvert Q_i^{hist} - Q_{total}\theta_i \rvert$ | 选中成员 ID |
| **Class 2** | 组内固定优先级选优 | $\arg\min_i \; Q_i^{hist}/\max(\theta_i,\epsilon)$ | 选中成员 ID |
| **Class 3** | 跨组动态归一化 + 水位消纳 | 每轮重算 $\theta_i^{cur} = Q_i^{due}/\sum Q_j^{due}$ | 分配明细记录 |

Class 3 消纳量约束（批量向上对齐，且永不击穿安全库存）：

$$Q_i^{actual} = \left\lceil \frac{\theta_i^{cur} \cdot Q_{net}}{\text{Lot}_i} \right\rceil \cdot \text{Lot}_i, \qquad
Q_i^{consumed} = \min\left(Q_i^{actual},\ \max(0, \text{OnHand}_i - \text{SS}_i),\ Q_{net}\right)$$

入口函数：`allocate_class1` / `allocate_class2` / `allocate_class3`。

### 已覆盖的边界与异常路径

场景覆盖不止于「正常能跑通」，以下分支都有可复现的测试数据与断言：

| 覆盖点 | 触发条件 | 可复现结果 |
| :--- | :--- | :--- |
| 交付**失败并回滚** | 需求 100 件 > 可用物料，窗口内无解 | 拦截 + 回滚 11 步，库存/产能零污染 |
| 交付**成功承诺** | 需求 30 件 @ Day 3 | 承诺 Day 3，预留 3 工时 |
| ITP **配额封锁** | 执行需求刻意放大 1.3×（`500k` 数据集中 20 万单越权） | 下派 30 万 / 阻断 20 万 |
| IOP **优先级抢占** | 多单竞争同一约束键 | 高优先级先占额度，低优先级被阻断 |
| 替代料**批量对齐** | 净需求 12、Class 3 批量倍数 5、安全库存 10 | 分配 $\lceil 6/5 \rceil \times 5 = 10$ |
| 替代料**安全库存保护** | 分配后库存逼近 `safety_stock` | 余额 90，安全库存 10 未被侵占 |
| 替代料**无解** | 组不存在或水位不足 | 退出码 `0` + 分配量 `0`（业务结论，非报错） |

> ⚠️ **已知边界**（见 [`ipc-core-benchmark/README.md`](ipc-core-benchmark/README.md)）：
> `allocate_class1` 的 `current_on_hand` 参数当前未参与选优，即一类替代暂未校验库存可行性；
> `part_id / 10` 是占位式物料族抽象，`std::hash<std::string>` 跨编译器结果不稳定，
> 生产环境应替换为确定性哈希（如 FNV-1a）。

---

## 二、性能

### 2.1 极限压测：200 万 SKU · 100 万需求

实测环境：**Apple M5 Pro / 64 GB / macOS 26.5.1 / Apple clang 21.0.0 / `-O2`**

数据规模（`data/`，由 `data/generate_data.py` 确定性生成）：

| 数据 | 行数 |
| :--- | ---: |
| `parts.csv`（物料-站点主数据） | 2,000,000 |
| `demands_master.csv`（ITP 主计划需求） | 500,000 |
| `demands_execution.csv`（IOP 执行需求，含 1.3× 超量插单） | 500,000 |

```
[Phase 1] CSV 加载 ............ 857.39 ms   (2,000,000 SKU + 1,000,000 需求)
[Phase 2] ITP 主计划 (500k) ... 17.88 ms    → 27,956,713 Demands/Sec
[Phase 3] IOP 执行计划 (500k) . 25.76 ms    → 19,406,956 Orders/Sec
────────────────────────────────────────────────────────────────
求解总延迟 .................... 43.65 ms
求解吞吐量 .................... 22,910,164 需求/秒
IOP 结果: 下派 300,000 / 阻断 200,000（配额消耗率 52.17%，阻断路径已覆盖）
```

复现命令：`./ipc-core-benchmark/bin/stress_benchmark_2m ipc-core-benchmark/data`

> **关键读法**：加载 857 ms 是**一次性 I/O 开销**；求解 43.65 ms 为**纯内存计算**。
> 两者必须分开看——引擎的价值在求解延迟，不在 CSV 解析。
> 200 万 SKU 全量装载后，求解器对 100 万需求的端到端延迟仍在**数十毫秒**量级。

### 2.2 单元基准：结论正确性

| 基准 | 场景 | 结果 |
| :--- | :--- | :--- |
| `test_delivery_precision` | 30 件 → 承诺 Day 3（预留 3 工时）；100 件 → 拦截并回滚 11 步 | ✅ PASS |
| `test_itp_iop_alignment` | 4 单（含 1 单越权插单）→ 下派 3 / 拦截 1，配额消耗率 93.94% | ✅ PASS |
| `test_substitution_rules` | Class 1 纠偏选中 ID 1；Class 3 分配 ⌈6/5⌉×5 = 10，安全库存 10 未被侵占 | ✅ PASS |

### 2.3 数学正确性对账

`verifier/` 提供**独立 Python 参考实现**，与 C++ 引擎逐案对账，验证数学逻辑一致：

```
[Validation 1] 一类替代料平摊配额   Python 选优=1  ↔  C++ 期望=1      → PASSED
[Validation 2] ITP/IOP 配额阻断     Python 3/1     ↔  C++ 期望 3/1    → PASSED
```

```bash
cd ipc-core-benchmark && ./build_and_run.sh                # 编译 + 三个单元基准 + 压测 + 交叉对账
cd ipc-core-benchmark/verifier && python3 cross_validator.py   # 仅数学对账
```

完整数学推导见 [`docs/MATHEMATICAL_SPEC.md`](ipc-core-benchmark/docs/MATHEMATICAL_SPEC.md)，
引擎原理与逐字段说明见 [`ipc-core-benchmark/README.md`](ipc-core-benchmark/README.md)。

---

## 三、输入数据契约

引擎的对外接口就是 **7 类 CSV**。数据为纯明文、可查看、可替换，由
`data/generate_data.py` **确定性生成**（纯函数推导、无随机），每次生成结果完全一致。

### 3.1 场景目录

一个目录 = 一份数据集。引擎只读取它需要的子集，缺失不相关的文件不影响运行。

```
my-scenario/
├── parts.csv                # 物料主数据（所有引擎）
├── bom.csv                  # 产品结构 / 替代料属性（delivery、substitution）
├── atp_supply.csv           # 供给节点（delivery）
├── capacity.csv             # 工作中心产能（delivery）
├── demands_master.csv       # 主计划需求（itp）
├── demands_execution.csv    # 执行需求（iop）
└── substitution_group.csv   # 替代料组定义（substitution）
```

| 引擎 | 必需文件 |
| :--- | :--- |
| `delivery` | `parts.csv` `bom.csv` `atp_supply.csv` `capacity.csv` |
| `itp` | `parts.csv` `demands_master.csv` |
| `iop` | `parts.csv` `demands_master.csv` `demands_execution.csv`（内部先生成 ITP 配额再跑 IOP） |
| `substitution` | `parts.csv` `bom.csv` `substitution_group.csv` |

### 3.2 字段契约

**表头必须逐字一致**（顺序一致、列名不可缺）。所有 ID 均为整数；空值不写。

| 文件 | 列 |
| :--- | :--- |
| `parts.csv` | `part_id,part_code,site,safety_stock,initial_on_hand,lot_size,lead_time` |
| `bom.csv` | `parent_id,child_id,usage_qty,alt_class,alt_group,target_ratio,historical_qty,lot_size` |
| `atp_supply.csv` | `supply_code,supply_type,part_id,available_day,qty,priority` |
| `capacity.csv` | `work_center,day,capacity_hours` |
| `demands_master.csv` | `demand_id,part_id,due_day,qty,priority,customer_group,region` |
| `demands_execution.csv` | 同上（列契约相同，语义不同） |
| `substitution_group.csv` | `alt_group,alt_class,member_part_id,target_ratio,historical_qty` |

逐字段类型与业务含义见 [`ipc-core-benchmark/data/README.md`](ipc-core-benchmark/data/README.md)。

### 3.3 直接驱动引擎：C++ CLI

`ipc_engine_cli` 为引擎提供稳定的「输入 CSV 目录 → 输出 CSV 目录」进程边界，
**不经过 demo、不经过 Node** 即可最小复现任何一个结论：

```bash
mkdir -p /tmp/out                       # 输出目录必须已存在
ipc-core-benchmark/bin/ipc_engine_cli --engine delivery \
    --in ipc-core-benchmark/data/demo/delivery-benchmark --out /tmp/out \
    --part-id 0 --due-day 3 --qty 30
cat /tmp/out/delivery_result.csv        # -> 0,3,30,1,true,3,30,3,0
```

```bash
# ITP 主计划配额
ipc-core-benchmark/bin/ipc_engine_cli --engine itp \
    --in ipc-core-benchmark/data/demo/itp-iop-benchmark --out /tmp/out

# IOP 刚性阻断（下派 3 / 阻断 1）
ipc-core-benchmark/bin/ipc_engine_cli --engine iop \
    --in ipc-core-benchmark/data/demo/itp-iop-benchmark --out /tmp/out

# 替代料三类决策（分配 10，安全库存 10 未侵占）
ipc-core-benchmark/bin/ipc_engine_cli --engine substitution \
    --in ipc-core-benchmark/data/demo/substitution-benchmark --out /tmp/out \
    --alt-class 3 --alt-group 3 --net-demand 12 --day 1
```

参数、输出 CSV 契约、退出码（`0` 成功 / `2` 参数错 / `3` 输入缺失 / `4` 计算失败）
见 [`ipc-core-benchmark/cli/README.md`](ipc-core-benchmark/cli/README.md)。

---

## 四、动手实验：用 Demo 理解引擎功能

> 这一节是**如何实验**的引导，不是项目主体的说明。目的是让业务使用者用**自己的数据**
> 跑通引擎，并看见需求、供给、资源、计划供给与决策结论，从而理解功能边界。
> 计算仍然发生在 C++ 引擎里——demo 只做编排与展示，**不实现任何引擎算法、不做二次计算**。

### 4.1 启动

```bash
cd ipc-demo
npm install
npm run demo
```

`npm run demo` 自动完成：依赖检查 → 编译 C++ CLI（产出 `bin/ipc_engine_cli`）→ 建 DuckDB
→ 导入全部场景输入 → 构建前端 → 启动服务。随后打开 <http://127.0.0.1:3001>。

> 环境要求：Node.js ≥ 20、C++17 编译器、Python 3（数据生成与交叉校验）、CMake（可选）。
> 重复启动会复用既存 DuckDB 与 CLI；只想跳过引导直接启动后端可用 `npm start`。

### 4.2 页面结构与推荐顺序

页面顶部是**场景下拉框**与 **6 个页签**：场景浏览 / 计划透视 / 交付承诺 / ITP / IOP / 替代料 /
性能与历史（下拉框全局生效，切换后所有页签跟着换数据集）。

内置 3 个基准场景，每个都对应上面的业务场景与性能结论：

| 场景 | 覆盖引擎 | 可复现的基准结论 |
| :--- | :--- | :--- |
| `delivery-benchmark` | delivery | 需求 30 件 @ Day3 → 承诺 Day 3、预留 3 工时；期望交期改 Day 1 → 承诺回退 Day 2、回滚 1 步 |
| `itp-iop-benchmark` | itp, iop | 下派 3 / 阻断 1，阻断原因「突破 ITP 配额上限」 |
| `substitution-benchmark` | substitution | 三类替代分配 10、安全库存 10 未被侵占 |

推荐顺序是**先跑三个会触发计算的页签，再用三个只读页签复核**：

| 顺序 | 页签 | 作用 |
| :--- | :--- | :--- |
| ① | 交付承诺 | 触发 ATP/CTP 计算，看承诺日与回滚过程 |
| ② | ITP / IOP | 触发配额与阻断计算，看防波堤是否生效 |
| ③ | 替代料 | 触发三类替代决策，看选料与安全库存保护 |
| ④ | 场景浏览 | 复核输入数据与历次运行的输出数据集 |
| ⑤ | 计划透视 | 复核指标的天粒度演进与单元格构成 |
| ⑥ | 性能与历史 | 复核每次运行的耗时与历史记录 |

下面逐个说明**具体怎么点**。

### 4.3 六个页签逐个操作指引

页面顶部的**场景下拉框**是全局限定的：切换场景后，下面所有页签都跟着换数据集。
右上角显示「引擎就绪 · DuckDB 就绪」表示 C++ CLI 与数据库都可用。
如果某场景缺少该引擎所需的数据集，页面会直接在顶部横幅里列出缺失项。

**① 交付承诺 —— 触发一次计算并复核结论**

1. 顶部场景下拉框选 `delivery-benchmark`，切到「**交付承诺**」页签；
2. 表单四个字段按顺序为**物料 (part_id) / 请求交期 (due_day) / 需求数量 (qty) / 优先级**，
   默认值即为基准组合 `0 / 3 / 30 / 1`，直接点运行即可；
3. 点「**运行交付承诺**」——按钮变为「运行中…」，页面提示「正在调用 C++ 引擎并导入 DuckDB…」，
   完成后原地回填结果；
4. 看顶部 KPI 卡：**可承诺 / 承诺日 / 承诺量 / 预留工时 / 回滚步数**
   （回滚步数 > 0 会以警示色高亮）；
5. 看**请求交期 vs 承诺交期时间轴**：橙色为请求交期，绿色 = 成功承诺、红色 = 失败；
6. 看**产能水位**与**BOM 层级展开**（逐层父件 → 子件用量）；
7. 用下方的**回放控件**（`← 后退` / 播放 / `前进 →` / `重置`）逐步重演每一步试算，
   每步旁边有 `note` 说明该日为何失败或成功；
8. **换个参数再跑一次**：把请求交期改成 `1`，重新点运行 —— 承诺日会回退到 Day 2、
   回滚步数变为 1，回放里能看到「Day 1 物料/产能不足，尝试次日」→「Day 2 可承诺」两帧。

> 第 8 步是这个页签最有价值的操作：它把引擎的**失败重试与回滚机制**变成可看见的过程，
> 而不是一个数字。

**② ITP / IOP —— 看配额防波堤与刚性阻断**

1. 场景下拉框选 `itp-iop-benchmark`，切到「**ITP / IOP**」页签；
2. 可调整**主计划缓冲系数 (buffer_factor)**（默认 1.10；基准场景下配额由它决定防波堤厚度）；
3. 点「**运行 ITP → IOP**」——这是一个串联动作，先跑 ITP 生成配额、再跑 IOP 做执行对齐，
   运行中页面会提示「先运行 ITP 生成配额，再运行 IOP…」；
4. 顶部聚合 KPI：**总订单数 / 已下派 / 已阻断 / 履行量 / 配额消耗率**
   （配额消耗率 ≥ 95% 会以警示色高亮）；
5. 在「**配额 vs 消耗（N）**」页签看四维约束键的授权上限与已消耗量，
   **临近/达到上限的行会被区分标色**；
6. 切到「**阻断订单（N）**」页签看被刚性拦截的订单明细与 `reason`
   （如「突破 ITP 配额上限（剩余 50 < 需求 300）」）；
7. 基准场景下应看到：总 4 单 → 已下派 3 / 已阻断 1，配额消耗率约 93.94%。

> 「配额 vs 消耗」是**为什么没超**，「阻断订单」是**谁超了、超在哪**——
> 两个页签配合看，才能验证防波堤完整生效。

**③ 替代料 —— 看三类规则如何选料并保护安全库存**

1. 场景下拉框选 `substitution-benchmark`，切到「**替代料**」页签；
2. 表单字段依次为**替代料类别 (1/2/3) / 替代料组 (alt_group) / 净需求 / 分配日 / 父物料**，
   **把默认的 `1 / 1 / 50 / 1 / 0` 改成基准组合 `3 / 3 / 12 / 1 / 0`**（类别改成 3）；
   页面上有一行提示说明三个类别的口径：类别 1 = 按目标比例分配、类别 2 = 按优先级/水位、类别 3 = 安全库存保护下分配；
3. 点「**运行替代料决策**」，完成后看决策 KPI：
   **类别 / 选中替代料 / 选择依据 / 净需求 / 本次分配合计 / 分配后在手**；
4. 看下方的**成员水位与安全库存保护带**：按成员展示目标比例、历史量、可分配水位与本次分配，
   **斜纹带 = 安全库存保护带（不可侵占）**，被选中的成员会高亮；
5. 基准组合应得到：选中成员 3、分配合计 10、分配后在手 90（安全库存 10 未被侵占）；
6. **试着把类别改成 1 或 2 再运行**，对比「选择依据」与选中成员的变化，
   就能直观看出三类替代策略的判据差异。

> 若分配合计为 0，页面会明确提示「不可分配：该类别在当前水位与安全库存约束下无可用替代料
> **（业务结论，非执行错误）**」——这正是引擎用退出码 `0` 返回业务结论的语义在界面上的体现。

**④ 三个只读复核页签：计划透视 / 场景浏览 / 性能与历史**

这三个页签不触发计算，但用来**核对**上面三个引擎的结论：

- **场景浏览** — 选场景后，在「输入数据浏览」按业务类别（计划供给 / 需求 / 供给 / 资源 /
  物料主数据 / 产品结构 / 替代料组）点数据集按钮看 DuckDB 明细；点「**查看原始 CSV**」
  逐字比对磁盘原文，点「返回表格」切回。下方「运行输出浏览」按 `run_id` 列出历次输出数据集。
  *回答：引擎吃进去的到底是什么数据、吐出来的是什么。*
- **计划透视** — 「**运行批次**」下拉可选「全部成功运行」或某一 `run_id`
  （平衡族指标按 `run_id` 取值）；用「筛选维度」+「维度成员值」过滤（如 `工作中心` = `WC_01`），
  点「清除筛选」复位；点族标题折叠，点任一数值单元格弹出**下钻明细**看构成该值的原始行。
  *回答：指标在时间轴上如何演进、单元格的值由哪些明细加总而来。*
- **性能与历史** — 上半部是「**历史运行耗时对比**」图，下半部「**运行列表（点击加载 run_id）**」
  可选任意一次历史运行；选中后看「**单次运行性能**」：
  **端到端耗时 / C++ 自报耗时 / 处理条数 / 吞吐量**，以及该次运行的输出数据集行数与元数据。
  *回答：每次运行有多快、历史上跑过什么、端到端耗时里有多少是编排开销。*

> **耗时口径**：端到端 = Node 编排 + CSV 往返 + 子进程启动；C++ 自报 = CLI 输出的 `duration_ms`。
> 两者对照着看，就能把「引擎本身的快」与「编排层的开销」分开。

**⑤ 每个页签都能回答一个问题**

| 页签 | 回答的问题 |
| :--- | :--- |
| 场景浏览 | 引擎吃进去的是什么数据、吐出来的是什么数据 |
| 计划透视 | 关键指标在时间轴上如何演进、单元格的值由哪些明细加总 |
| 交付承诺 | 引擎凭什么给出这个承诺日、失败时如何回滚 |
| ITP / IOP | 配额防波堤为什么没被突破、谁突破被拦了 |
| 替代料 | 三类规则如何选料、安全库存有没有被侵占 |
| 性能与历史 | 每次运行有多快、历史上跑过什么 |

三个会触发计算的页签跑完后，页面上都会给出本次运行的标识：
交付承诺底部有完整的**运行元数据**区块（`run_id` / 端到端耗时 / C++ 自报耗时 / 退出码 / 参数），
ITP、IOP 分别显示配额与执行两条 `run_id`，替代料在决策结论标题旁显示短 `run_id`。
**记下 `run_id`**，它就是到「性能与历史」页签回查这次运行的凭据。

输入与输出 CSV 原文同时归档，可按 `run_id` 无重算回放历史结论。

### 4.4 用自己的数据实验

把下面 7 个 CSV 放进一个目录即可作为一个场景（**文件名必须完全一致**），
引擎只读取它需要的子集。三种方式：

**A · 以内置场景为模板**（最快）——复制一个内置场景并替换 CSV：

```bash
cp -R ipc-core-benchmark/data/demo/delivery-benchmark /tmp/my-scenario
# 编辑 /tmp/my-scenario/*.csv 换成自己的数据
```

**B · 通过 API 新建场景**（Demo 运行中，写入可写场景根 `ipc-demo/scenarios/`）：

```bash
curl -X POST http://127.0.0.1:3001/api/scenarios \
     -H 'content-type: application/json' -d '{"scenarioId":"my-scenario"}'
curl -X PUT http://127.0.0.1:3001/api/scenarios/my-scenario/datasets/parts \
     -H 'content-type: text/csv' --data-binary @parts.csv
# …其余数据集同理；data key 见 API 契约
curl -X POST http://127.0.0.1:3001/api/scenarios/my-scenario/import
```

**C · 放入场景根目录随启动自动发现**——放进 `SCENARIO_ROOT`（默认
`ipc-core-benchmark/data/`，含 `data/demo/`），重启 Demo 即被导入。
内置场景根视为**只读**，自建场景请用方式 B 的可写根。

> **校验提示**：场景列表会标注每个引擎的**可运行性**；显示「不可运行」时会直接标出缺失的数据集或列。

### 4.5 Demo 的错误语义

| 状态 | `kind` | 含义 |
| :--- | :--- | :--- |
| `400` | `invalid_request` | 参数非法（不启动子进程） |
| `422` | `scenario_incomplete` | 场景缺少该引擎所需数据集 |
| `503` | `engine_missing` | C++ 可执行文件缺失/不可执行 |
| `504` | `engine_timeout` | 子进程超时 |
| `502` | `engine_nonzero_exit` | 子进程非零退出 |
| `502` | `malformed_output` | 输出 CSV 缺失或列不全 |

> **业务失败不等于报错**：例如替代料在当前水位下不可分配，引擎以退出码 `0` 返回、
> 分配量为 `0`，接口返回 `201` + 业务结论。只有**编排层面**的失败才是错误。
> 完整 API 契约见 [`ipc-demo/README.md`](ipc-demo/README.md)。

### 4.6 一条命令验证全部结论

```bash
cd ipc-demo && npm test                            # Node：场景/持久化/透视一致性/API/前端组件
./ipc-core-benchmark/cli/cli_regression.sh         # C++ CLI 四引擎回归
cd ipc-demo && node test/browser/e2e.mjs           # 真实前端页面端到端（需 Demo 已启动）
```

---

## 五、目录结构

```
Value-Chain-Physics/
├── README.md                          # 本文档（引擎业务场景覆盖与性能）
├── CODE_ARCHITECTURE_AND_FLOWS.md     # 代码架构与流程说明
│
├── ipc-core-benchmark/                # ★ 核心：C++ 计算内核（唯一计算源）
│   ├── README.md                      # 引擎原理、数理规范、基准结果
│   ├── include/ipc_core/              # 对外接口（头文件即契约）
│   ├── src/                           # 4 个引擎实现
│   ├── benchmarks/ verifier/ docs/    # 基准、Python 交叉对账、数理白皮书
│   ├── cli/                           # CSV 输入/输出 CLI + 回归脚本
│   └── data/                          # 测试数据
│       ├── README.md                  # 数据字典
│       ├── demo/                      # 基准场景（delivery / itp-iop / substitution）
│       └── sample/                    # 迷你数据集
│
└── ipc-demo/                          # 辅助：Node.js 功能实验与可视化工具
    ├── README.md                      # 启动、场景组织、API 契约、视图说明
    ├── src/server/ routes/ db/ engine/ scenario/ pivot/
    ├── src/web/                       # Vite + React 前端 SPA
    ├── scripts/demo.mjs               # 一键引导启动
    └── test/                          # 单元/集成测试（含 test/browser 页面端到端）
```

---

## 六、深入阅读

| 文档 | 内容 |
| :--- | :--- |
| [`ipc-core-benchmark/README.md`](ipc-core-benchmark/README.md) | ★ 4 个引擎的原理、入口函数、输入输出、核心公式、基准结果、数学对账 |
| [`ipc-core-benchmark/docs/MATHEMATICAL_SPEC.md`](ipc-core-benchmark/docs/MATHEMATICAL_SPEC.md) | 数理规范白皮书（完整数学推导） |
| [`ipc-core-benchmark/data/README.md`](ipc-core-benchmark/data/README.md) | 字段级数据字典与数据生成规则 |
| [`ipc-core-benchmark/cli/README.md`](ipc-core-benchmark/cli/README.md) | C++ CLI 参数、输入/输出 CSV 契约、退出码、约束键约定 |
| [`ipc-demo/README.md`](ipc-demo/README.md) | Demo 架构、场景管理、配置项、后端 API 契约、前端视图 |
| [`CODE_ARCHITECTURE_AND_FLOWS.md`](CODE_ARCHITECTURE_AND_FLOWS.md) | 代码架构与关键流程 |

---

## 七、构建与边界

### 构建

| 项目 | 要求 |
| :--- | :--- |
| 编译器 | 任意支持 C++17 的编译器（MSVC / GCC / Clang） |
| 第三方依赖 | **无**（零外部库，仅标准库） |
| Python（可选） | 3.7+（数据生成与交叉校验） |
| CMake（可选） | 3.14+（亦可直接用脚本编译） |

```bash
python3 ipc-core-benchmark/data/generate_data.py            # 生成全量数据 (~145 MB)
python3 ipc-core-benchmark/data/generate_data.py --sample   # 仅迷你集 (~5 KB)
cd ipc-core-benchmark && ./build_and_run.sh                 # 编译 + 全部基准 + 交叉校验
```

**已验证平台**：macOS (Apple clang 21)、Windows (MSVC 2017+)、Linux (GCC 9+)。

### 边界与约束

- **计算归属**：算法**只在 C++ 引擎内**实现；Node 与前端不得重算约束键或指标，不做二次计算。
- **不改动既有内核**：`ipc-core-benchmark/` 的引擎源码、头文件契约、既有基准程序与
  既有数据文件保持不变；本仓库新增 CLI 与 `data/demo/` 基准场景。
- **内置数据只读**：内置场景根为只读；自建场景写入 `ipc-demo/scenarios/`。
- **Demo 定位**：面向**本地演示与评审**，默认绑定 `127.0.0.1`，**不设计为生产多租户服务**。
- **系统覆盖范围**：IPC 完整覆盖 IBP → ITP → IOP → 车间排程的端到端闭环。
  本仓库开放 4 个脱敏核心算子与基准集，用于证明「百万级规模毫秒级求解」在物理与数学上可达。
  生产环境全量商业代码库（DuckDB 物理适配层、大模型预测中枢、完整商业控制塔）
  因**专利申报中**暂未开源。

本项目采用 [Apache 2.0 License](ipc-core-benchmark/LICENSE) 协议开源。
