# Design

## Context

现有 IPC 核心以 C++17 微内核形式存在于 `ipc-core-benchmark/`：4 个算子、统一 POD 数据契约（`include/ipc_core/types.h`）、CSV 数据集（`data/`，另含 `data/sample/`）、3 个单元基准、1 个压测与 Python 参考对账。数理语义集中在 `docs/MATHEMATICAL_SPEC.md`。

**现状关键事实（影响架构）**：

- 三个单元基准（`test_delivery_precision`、`test_itp_iop_alignment`、`test_substitution_rules`）把数据**硬编码在 `main()` 内**，无 `argc/argv`，输出是终端文本 + `assert`，**不产出任何 CSV**。
- `stress_benchmark_2m` 能读 CSV 目录（`argv[1]`），但只向 `stdout` 打印人类可读文本，同样**不产出 CSV**。
- 既有 4 个算子（`promise_delivery_date`、`generate_itp_master_allotments`、`run_iop_execution_alignment`、`allocate_class1/2/3`）返回的是**聚合结果**：ATP 返回单个 `ATPQueryResult`；ITP 返回 `unordered_map<AllotmentConstraintKey, AllotmentState>`；IOP 返回聚合计数（总/下派/阻断/履行量/消耗率），**不返回逐单下派/阻断明细**；替代料返回 `AlternateAllocationRecord` 列表与选中 ID。
- `itp_iop_alignment.cpp` 中 `family_id = part_id / 10`、`cust_group_id = hash(customer_group) % 100`、`region_id = hash(region) % 10` 是既有映射约定，Node 与前端需与之一致。
- `data_loader.h` 提供了 CSV 读取能力，但其 `read_csv` 是朴素逗号切分（不处理引号/转义），**不适合作为输出写出**；CSV 写入需新写。

约束：

- 用户指定：**Node.js 调用 C++ 引擎做计算；C++ 读 CSV、输出 CSV；Node 负责把输入/输出 CSV 导入 DuckDB 供前端分析。**
- 既有引擎源码、头文件契约、既有基准程序与数据文件**不得被修改**（新增 CLI 不得改变既有行为）。
- 演示需复现既有基准结论（需求 30 件 @ Day3 → 承诺 Day 3、预留 3 工时；ITP/IOP 下派 3 / 阻断 1；三类替代料分配 10）。
- Demo 面向本地演示/评审，默认仅绑定回环地址。

参考：动机与范围见 `proposal.md`；行为契约见 `specs/ipc-demo/*/spec.md`。

## Goals / Non-Goals

**Goals:**

- 在 C++ 侧新增 CSV-in / CSV-out CLI 驱动，作为 Node 与 C++ 之间**唯一且稳定的进程边界**，且不触碰既有算子与基准。
- Node 后端仅做编排与数据管道（写输入 CSV → 调子进程 → 读输出 CSV → 入 DuckDB），**不实现任何引擎算法**。
- 以「测试场景」为数据组织单位：每个场景是自包含目录，含本次计算所需的全部数据集。
- 场景的**输入数据集**与运行产出的**输出 CSV** 全部入库，用户可在同一界面统一查看需求、供给、资源、计划供给与计算结果。
- 提供 SAP IBP 风格的计划透视图：天粒度时间桶 × 关键指标，供跨时间分析。
- 每次运行的输入/输出 CSV 与元数据按 `run_id` 归档到 DuckDB，前端可 SQL 化查询、对比与回放。
- 子进程失败/超时/产物缺失等异常被如实识别与上报，不产生虚假成功。
- 一条命令完成「构建 C++ → 建库 → 导入样例 → 启动」。

**Non-Goals:**

- 不修改 `atp_ctp_engine.cpp`、`itp_iop_alignment.cpp`、`substitution_engine.cpp`、`types.h` 及既有基准程序。
- 不实现完整的 IBP/APO 计划引擎能力（如多级 MRP 净需求推算、能力均衡优化、多版本计划快照对比）——透视视图展示的是**既有引擎数据所能支撑**的指标。
- 不对输入 CSV 做业务规则级校验（仅做列契约与可解析性校验）。
- 不在 Node 侧重实现或"模拟"引擎逻辑（原设计中的移植方案已废弃）。
- 不改动 `MATHEMATICAL_SPEC.md` 定义的数学语义。
- 不做生产级鉴权、多租户、水平扩展；不做移动端适配。
- 不承诺 Demo 复刻 C++ 在 2M 规模下的毫秒级吞吐（Node 编排 + CSV 往返有额外开销）。

## Decisions

### 1. 计算归属：C++ 引擎为唯一计算源，Node 为纯编排层

C++ 保持计算权威性；Node 不复制算法。

- **理由**：这是用户的明确要求，也消除了"移植导致语义漂移"这一最大风险——数值结果直接来自被对账过的既有 C++ 实现。原设计中"Node 原生移植 + 对拍测试"的必要性随之消失。
- **替代方案**：Node 原生重实现（原方案）—— 废弃，会引入双实现漂移风险且违背用户指定。
- **替代方案**：N-API / WASM 嵌入 C++ —— 否决，构建复杂度高，且进程内嵌入会让子进程超时/隔离语义更难表达。
- **结果**：性能相关的能力从"Node 求解吞吐"改为"Node 编排端到端耗时 + C++ 自报耗时"，两者都需记录（见决策 6）。

### 2. C++ CLI：新增独立入口文件，不改既有工程结构

在 `ipc-core-benchmark/` 下新增一个 CLI 入口（如 `src/ipc_cli_main.cpp`，或置于新目录 `cli/`），**新增**而非改造既有 `main()`。通过 CMake 与 `build_and_run.sh` 增加构建目标。

调用形态设计为：

```
ipc_engine_cli --engine <delivery|itp|iop|substitution> \
               --in <input_dir> --out <output_dir> \
               [--buffer-factor 1.10] [--timeout 表意参数]
```

- **理由**：
  - 既有基准把数据硬编码，无法参数化，因此复用它们不可行；新增入口能以 `arvg` 接收 CSV 目录。
  - 不改既有文件 → 满足"既有行为不变"的 spec 要求，且不破坏已有对拍/评审基线。
  - 以**目录**而非单文件为参数边界，可让多输入引擎（ITP 需 parts + demands_master；IOP 需 parts + demands_execution + ITP 配额）自然扩展，也便于 Node 侧每次运行一个独立目录。
- **替代方案**：改造既有基准接受 argv —— 否决，会改变既有程序行为并可能破坏断言输出一致性。
- **替代方案**：单个 CLI 一次性跑完全部引擎 —— 否决，可视化需要按引擎独立触发；但 CLI 可按 `--engine` 分派，保留"一次一引擎"的简单语义。

### 3. 输出 CSV schema：每个引擎一个明确契约

CLI 为每类引擎写出确定性命名的 CSV，Node 按固定 schema 解析：

- `delivery`: `delivery_result.csv`（是否可承诺、承诺日、承诺数量、产能占用、回滚步数），另附 `delivery_steps.csv` 记录逐层 BOM/供给/产能试算与回滚步骤，供前端逐步回放。
- `itp`: `itp_allotments.csv`（`day,family_id,cust_group_id,region_id,total_quota,consumed_qty`）。
- `iop`: `iop_orders.csv`（逐单：`demand_id,part_id,due_day,qty,priority,family_id,cust_group_id,region_id,status,reason`）与 `iop_summary.csv`（聚合指标）。**逐单明细是新增能力**，因为既有 `IOPExecutionResult` 只返回聚合计数——这是让前端能"标出被阻断订单"的必要条件。
- `substitution`: `substitution_allocations.csv`（分配记录）与 `substitution_decisions.csv`（各类别被选中成员与依据）。

- **理由**：稳定、可版本化、可被 DuckDB `read_csv_auto` 直接导入的契约，是 Node/C++ 解耦的前提。逐单明细是可视化需求的直接产物，必须在 CLI 侧生成而非 Node 侧反推。
- **权衡**：为产出逐单明细，CLI 需要在**不修改既有算子**的前提下记录其决策。做法是在 CLI 内复用算子并按引擎约定重建可观测明细（例如 IOP 的逐单判定的判定条件与算子内部一致：`remaining_quota >= qty` 则下派，否则阻断；约束键按决策 4 的映射计算）。这是"重新表达算子已有判定"，**不是**重新实现求解逻辑；风险与缓解见 Risks。
- **替代方案**：修改算子签名以返回明细 —— 否决，违反"不修改既有引擎"约束。
- **替代方案**：Node 侧根据聚合结果反推明细 —— 否决，会变成 Node 侧重实现逻辑。

### 4. 约束键映射与数值一致性

Node 与 CLI 必须对 `family_id = part_id / 10`、`cust_group_id = hash(customer_group) % 100`、`region_id = hash(region) % 10` 使用**完全一致**的计算。

- **风险**：`std::hash<std::string>` 的实现是**标准库实现相关**的（libstdc++/libc++/MSVC 各不相同）。Node 无法可靠复现 C++ 的 `std::hash`。
- **决策**：**不把约束键映射放到 Node**。ITP 输出 CSV 直接写出算子里已成型的 `family_id/cust_group_id/region_id` 值（CLI 在生成防波堤后遍历 map 输出）；IOP 输出 CSV 的逐单记录的键由 CLI 用**与算子在同一个二进制、同一个标准库**下计算得出。Node 只做 CSV 透传与入库，绝不自行计算这些键。
- **理由**：这将"跨语言哈希一致性"问题彻底消除，而不是靠约定去凑。前端展示的是 C++ 产出的键值，天然一致。
- **替代方案**：在 Node 中实现各平台 `std::hash` —— 否决，脆弱且不可移植。

### 5. CSV 写出：CLI 侧自行实现最小写入器，不用既有 read_csv 写回

CLI 新增一个最小的 CSV 写出工具（正确处理逗号、引号转义、CRLF/LF），不依赖 `data_loader.cpp` 的 `read_csv`（后者仅用于读，且为朴素切分）。

- **理由**：既有 `read_csv` 明确不处理引号转义，不可用于可靠输出；字段中有 `part_code`/`customer_group` 等字符串，虽样例数据无逗号，但契约应稳健。
- **替代方案**：直接 `std::cout` 拼接 —— 否决，缺乏转义且难以定位产物。
- **替代方案**：引入第三方 CSV 库 —— 否决，既有工程为零依赖，保持零依赖。

### 6. Node 编排：独立运行目录 + 子进程 + 严格校验

每次运行创建隔离工作目录（如 `<workroot>/<run_id>/in/` 与 `.../out/`）：

1. 解析所选场景，按 CLI 契约准备输入（默认可直接以场景目录为输入；仅当请求覆盖场景参数时才把覆盖后的数据写入运行时副本目录），并把该场景的输入数据导入 DuckDB。
2. `spawn` C++ 可执行文件，参数指向 `in`/`out` 目录；施加超时；捕获 stdout/stderr 与退出码。
3. 仅当**退出码为 0 且输出 CSV 存在且必需列可解析**时，才认定为成功结果。
4. 将输出 CSV 逐行导入 DuckDB 结果表；同时归档原始输入/输出 CSV 内容与运行元数据。

- **理由**：隔离目录解决并发互相覆盖；`spawn`（非 `exec`）避免 shell 注入并支持流式捕获；"退出码+产物"双重校验防止把失败当成功。
- **替代方案**：`exec` 拼命令行 —— 否决，注入风险与转义麻烦。
- **替代方案**：常驻子进程 + 管道 RPC —— 否决，演示场景下复杂度不划算，且 CSV 边界正是用户指定。
- **并发**：Node 单进程内可并发多个 `spawn`；默认对同时运行的子进程数设上限，避免整机被压垮。

### 7. DuckDB：唯一持久与分析层，分层 schema

单文件数据库（默认 `ipc-demo/.data/ipc_demo.duckdb`），通过 Node 绑定（`@duckdb/node-api`，备选 `duckdb`）访问。导入用 DuckDB 原生 `read_csv_auto`（对全量 ~145 MB CSV 远快于 JS 逐行插入）。

Schema 分层：

- **基础数据集表**：`parts`、`bom`、`capacity`、`atp_supply`、`demands_master`、`demands_execution`、`substitution_group`，列与既有 CSV 契约一致；`dataset_imports` 记录来源标识（sample/full）、行数、导入时间。
- **运行元数据表**：`engine_runs`（`run_id`、`engine_type`、`created_at`、`params_json`、`exit_code`、`duration_ms`、`engine_duration_ms`、`status`、`error_message`、`dataset_source_id`、`executable_path`、`in_dir`、`out_dir`）。
- **结果明细表**：`delivery_result`、`delivery_steps`、`delivery_bom`、`itp_allotments`、`iop_orders`、`iop_summary`、`substitution_allocations`、`substitution_decisions`、`substitution_water`，均带 `run_id`（与 CLI 的 9 个输出 CSV 一一对应）。
- **原始产物**：`run_artifacts`（`run_id`、`direction`（in/out）、`filename`、`content` 或归档路径），满足"保留原始 CSV"的 spec。

- **理由**：明细表支撑 SQL 查询与历史对比；`run_artifacts` 确保审计与可重导入；`engine_runs` 同时记录 Node 端到端耗时与 C++ 自报耗时，使性能视图能拆分"引擎算得快"与"编排+CSV 往返开销"。
- **替代方案**：Postgres/SQLite —— 否决，用户明确要求 DuckDB。

### 8. 测试场景：目录即场景，且直接作为 C++ 输入目录

**决策**：场景 = 目录。一个场景目录含 7 类数据集 CSV，且**该目录可直接作为 C++ CLI 的 `--in` 参数**，无需格式转换。

- **理由**：
  - 用户明确要求"每个测试场景一个目录，包含本次计算需要的所有数据集"。
  - 既有 `data/` 与 `data/sample/` 天然就是这个结构，所以「场景」概念与既有数据组织**同构**，`sample/` 和 `data/` 根目录直接成为两个初始场景，不产生数据搬运或复制。
  - 场景目录直接作为 CLI 输入，消除了"Demo 格式"与"引擎格式"两套契约，Node 侧也无需为 C++ 生成另一份输入（仅当请求覆盖场景参数时才写运行时副本）。
- **场景根目录可配置**（`SCENARIO_ROOT`，默认指向 `ipc-core-benchmark/data/`）：用户可在仓库外自建场景目录集合，不污染既有数据。
- **只读约束**：引擎以只读方式使用场景目录，Node 不得在运行中改动场景文件（spec 有对应场景断言），避免"跑一次改一次数据"的隐性状态。
- **新建场景的写入位置**：新建场景默认写入场景根目录下；若场景根指向仓库内既有 `data/`，新建场景应写入 Demo 自己的场景目录（`ipc-demo/scenarios/`）以免改动仓库既有数据文件——实现时以「可写场景根」与「只读内置场景根」两者并存的方式处理。
- **内置 Demo 场景落点**：随仓库提供的、用于复现基准结论的**新增**预设场景统一放在 `ipc-core-benchmark/data/demo/` 下（每个场景一个子目录，如 `data/demo/delivery-benchmark/`）。该目录属于**新增**，`data/` 根与 `data/sample/` 保持逐字节不变，仅作为只读内置场景被扫描发现。
- **为什么必须新增预设场景**：既有 `data/sample/` 无法复现交付基准结论（承诺 Day 3 / 预留 3 工时），原因是数据而非引擎或 CLI——(1) sample 中查询物料 0 的唯一供给是 `available_day=5` 的计划订单，Day3 前无可用供给；(2) sample 的 `bom.csv` 把替代料成员（`alt_class=1/1/3`）与真实 BOM 子件混在 `parent_id=0` 下，而既有 `reserve_atp_and_capacity_recursive` 对所有 `parent_id` 匹配的 BOM 行一律递归（不按 `alt_class` 过滤）。基准 `test_delivery_precision.cpp` 使用的是**硬编码内存数据**（仅子件 B 有 50 库存），与 sample CSV 本就不同。
  - 因此**不改引擎、不改 CLI 过滤逻辑、不改 sample 数据**，而是新增 `data/demo/delivery-benchmark/` 使其数据与基准的硬编码场景等价（BOM 仅含 `alt_class=0` 的真实子件、子件 B 期初/在手 50、产能从 Day0 起 100 工时）。验证：CLI 对该场景返回 `promised_day=3, total_capacity_used=3, rollback_steps=0`。
  - 其余三个引擎的预设场景同样置于 `data/demo/` 下（`itp-iop-benchmark/`、`substitution-benchmark/`），各自自包含七类数据集。
- **替代方案**：在 CLI 中按 `alt_class==0` 过滤 BOM 递归 —— 否决，这会改变既有算子行为（`promise_delivery_date` 仍会递归替代料行），导致 CLI 结论与引擎权威结论不一致，违背"复用既有算子、不改语义"的决策 2。
- **替代方案**：把场景定义存成数据库里的 JSON 配置，运行时再物化 CSV —— 否决，凭空增加一层间接，且违背"目录即场景"的直观性。

### 9. 输入数据入库：与输出同等待遇的一等数据

**决策**：场景的 7 类**输入** CSV 与运行的**输出** CSV 都导入 DuckDB，输入数据按 `scenario_id` 归属，输出按 `run_id` 归属。

- **理由**：用户明确要求"输入数据 csv 也导入到 duckdb 供用户查看，这样用户统一可以看到需求、供给、资源、计划供给等信息"。src/C++ 引擎读的是 CSV，若只在 DuckDB 里放结果，用户就无法在界面里核对引擎看到了什么——输入入库使"输入 → 计算 → 输出"在同一个可查询平面内闭合。
- **schema 影响**：基础数据集表（`parts`/`bom`/`capacity`/`atp_supply`/`demands_master`/`demands_execution`/`substitution_group`）增加 `scenario_id` 列作为归属；`dataset_imports` 记录 `scenario_id` + 来源路径 + 行数 + 导入时间。
- **幂等性**：重复导入同一场景的数据集需不产生重复行 —— 实现为按 `scenario_id` 先删后插（或 `CREATE OR REPLACE`），而非依赖主键去重，因为 `bom`/`capacity`/`atp_supply` 等表在多场景下没有天然全局唯一键。
- **替代方案**：只在运行时读 CSV、不落库 —— 否决，违背用户要求且前端要直连文件；把输入数据只作为临时表 —— 否决，无法支持场景切换与跨运行对比查询。

### 10. 计划透视：以 SQL 视图为主要实现，前端只做呈现

**决策**：关键指标的天粒度透视**在后端用 DuckDB SQL（视图/查询）计算**，返回「指标 × 天」的规整结构；前端不自行推算指标。

- **指标依引擎业务场景确定**，分四族（详见 `specs/ipc-demo/planning-pivot/spec.md`）：
  - **供给族**：在手、在途(SR)、计划订单、按天累计计划可用量 —— 源自 `atp_supply`（`supply_type` 与 `available_day`）。
  - **需求族**：主计划独立需求、执行需求、毛需求 —— 源自 `demands_master` 与 `demands_execution`（`due_day`）。
  - **资源族**：可用产能、已占用产能、剩余产能（负值即超载）—— 源自 `capacity` 与交付承诺运行的产能占用结果。
  - **平衡族**：已承诺量、配额 vs 消耗、阻断量、缺口/积压 —— 源自交付承诺与 ITP/IOP 运行结果。
- **时间轴**：由场景数据实际涉及的天数范围推导（需求 `due_day`、供给 `available_day`、产能 `day` 的并集区间），保证不因硬编码范围而截断数据。
- **空值语义**：`NULL`（无计划）与 `0`（计划为零）必须在透视中可区分，因此 SQL 聚合避免用 `COALESCE(...,0)` 抹平空值。
- **一致性**：资源族的"已占用产能"必须与交付承诺结果一致、平衡族的配额/阻断必须与 ITP/IOP 结果一致（spec 有对应用例）——通过让透视直接读运行结果明细表来保证，而非另算一套。
- **替代方案**：前端拉原始明细自行聚合 —— 否决，会在前端复制业务口径且难以保证与引擎结果一致；把指标固化进输出 CSV —— 否决，透视需要跨运行、跨数据族的组合，超出单次输出范畴（输出 CSV 仍保留为权威明细）。

### 11. 前端：Vite + React + 声明式图表，数据全部经后端 API

前端为 Vite 构建的 SPA（React + TypeScript），ECharts 或 Recharts 绘图；所有数据经后端 API（底层 DuckDB）获取，**前端不直接访问 C++ 或 CSV 文件**。

- **回放机制**：`delivery_steps` 等明细帧由后端一次性返回，前端维护 `cursor` 做前进/后退/播放/重置，避免逐帧往返。
- **理由**：客户端回放流畅；统一数据出入口便于权限与格式治理。
- **替代方案**：前端直连 DuckDB-WASM —— 否决，权限路径分叉且全量数据不适合下发。

### 12. 引导：一条命令含 C++ 构建

`npm run demo`（或 `npm start`）执行：检查依赖 → 构建 C++ CLI（若可执行文件缺失，调用 `clang++`/CMake）→ 建库/迁移 → 空库时导入样例 → 启动前后端。

- **理由**：spec 要求首次运行一条命令可跑通；C++ 可执行文件是运行前提，必须纳入引导而非让用户手动编译。
- **默认绑定 `127.0.0.1`**；环境变量 `PORT`、`DUCKDB_PATH`、`DATA_DIR`、`ENGINE_BIN`、`ENGINE_TIMEOUT_MS` 覆盖。
- **替代方案**：要求用户先手工 `./build_and_run.sh` —— 否决，不满足一条命令目标。

### 13. 全量数据定位

全量导入作为可选能力（`npm run import:full`），用于演示数据规模与聚合查询；交互式求解默认面向样例/中等规模。

- **理由**：Node 编排 + CSV 往返回合在 2M 规模下不具交互性；用户诉求是可视化演示而非复刻压测性能。

## Risks / Trade-offs

- [CLI 重建逐单明细时与算子判定不一致] → 逐单明细必须严格复用算子的判定条件与同一二进制内的键映射（决策 4）；用既有基准场景做回归（ITP/IOP 必须得到下派 3 / 阻断 1），并断言明细计数与算子聚合结果完全相等。
- [跨平台 `std::hash` 差异导致键不一致] → 键一律在 C++ 侧计算并输出（决策 4），Node 不参与；在文档中明确此约定。
- [子进程产物不完整被误判为成功] → 强制"退出码 0 且必需列齐全"双重校验；输出目录预期文件缺失即判失败。
- [并发运行互相覆盖] → 每次运行独立 `in/out` 目录，并对并发子进程数设上限。
- [全量 CSV 导入耗时/占盘] → 导入与启动路径解耦，仅在显式命令下执行；样例路径不触发全量读取。
- [Node/DuckDB 绑定平台差异或 API 变动] → DuckDB 访问封装到单一 repository 层；锁定依赖版本并在 README 说明平台前置要求。
- [新增构建目标影响既有构建] → 新增目标而非改既有目标；验收包含"既有 4 个基准运行结果不变"的检查。
- [性能表述混淆] → 性能视图分别展示 C++ 自报耗时与端到端耗时，避免把编排开销算作引擎性能。
- [多场景数据在基础表中混淆] → 所有基础数据集表带 `scenario_id`，查询一律按场景过滤；透视与浏览接口都强制要求场景参数。
- [场景库被运行过程污染] → 引擎只读场景目录；新建场景默认落到 Demo 自有可写场景根，避免改动仓库既有数据。
- [透视指标与引擎结果不一致] → 资源与平衡族指标直接读运行结果明细表计算，并设一致性用例（产能占用对齐交付承诺、配额/阻断对齐 ITP/IOP）。
- [透视数据量与前端渲染压力] → 透视按场景 + 维度筛选后才返回，时间范围限定在该场景实际天数区间；大场景下按物料/工作中心分批下钻。
- [空值与零值被抹平] → SQL 聚合中禁止用 `COALESCE(...,0)` 填充缺失计划；前端将 `NULL` 渲染为空白并单测该行为。

## Migration Plan

变更为**以新增为主**：新增 C++ CLI 入口与构建目标、新增 `ipc-demo/`，不修改既有引擎、头文件契约、基准程序与数据文件。

1. 新增 C++ CLI 入口与构建配置，验证既有 4 个基准输出未变。
2. 新增 `ipc-demo/`；`.data/`、运行工作目录与 `node_modules/` 加入忽略规则。
3. 首次运行由引导脚本构建 CLI、创建 DuckDB、导入样例数据。
4. 回退：删除新增的 CLI 入口/构建目标与 `ipc-demo/` 目录及生成的数据库文件即可；既有内核与数据不受影响。

## Open Questions

无。框架、图表库与 DuckDB 绑定的具体版本可在实现阶段按当时情况微调，均不影响 specs 定义的行为与任务分解。
