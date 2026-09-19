# IPC 优化引擎可视化演示（ipc-demo）

Node.js 演示应用：**Node 编排层调用既有 C++ IPC 引擎做计算**，把测试场景的**输入 CSV**
与每次运行的**输出 CSV** 一并导入 **DuckDB**，供前端在统一界面中浏览需求、供给、资源、
计划供给与计算结果，并提供 SAP IBP 风格的天粒度计划透视图。

> 计算归属：**C++ 引擎是唯一计算源**。Node 只负责准备输入 CSV、以子进程调用 CLI、
> 读取输出 CSV、导入 DuckDB 与提供 API；**Node 不实现任何引擎算法**。

---

## 1. 快速开始（一条命令）

```bash
cd ipc-demo
npm install
npm run demo          # 依赖检查 → 构建 C++ CLI → 建库 → 导入场景输入 → 构建前端 → 启动
```

随后打开 <http://127.0.0.1:3001>。`npm run demo` 在首次运行时会调用
`ipc-core-benchmark/build_and_run.sh` 编译出 `bin/ipc_engine_cli`，并导入
`ipc-core-benchmark/data/demo/` 下的全部测试场景。

**重复启动**：若 DuckDB 与 C++ CLI 已存在，脚本复用既存数据（不清空），仅重建前端并启动服务。
只启动服务（跳过引导）可用：

```bash
npm start             # 直接启动后端（含静态前端），首次仍会自动导入场景输入
```

### 环境要求

| 组件 | 版本 | 说明 |
| :--- | :--- | :--- |
| Node.js | ≥ 20 | 使用内置 `node:test`、`fetch`、ESM |
| C++ 编译器 | clang++ / g++，支持 C++17 | 构建既有引擎与新增 CLI |
| Python 3 | 3.x | 仅在 `build_and_run.sh` 的交叉校验步骤使用 |
| CMake | 可选 | 仅在使用 CMake 构建 CLI 时需要 |

---

## 2. 架构

```
┌────────────┐   ① 复制/写出输入 CSV   ┌──────────────────────────┐
│  测试场景   │ ─────────────────────▶ │ C++ CLI (ipc_engine_cli) │
│ (目录=场景) │                        │ 既有 4 个算子，读/写 CSV  │
└────────────┘                        └────────────┬─────────────┘
       │                                           │ ② 输出 CSV
       │ ③ 输入入库                                  ▼
       ▼                                 ┌────────────────────┐
┌──────────────────────────┐  ④ 输出入库  │  Node 编排层        │
│        DuckDB            │ ◀─────────── │ (src/engine/*)      │
│ 输入表 + 运行结果表 + 元数据 │             └────────────────────┘
└────────────┬─────────────┘                        │
             │ ⑤ SQL 查询/透视                        │ 子进程 spawn + 超时/并发
             ▼                                      ▼
┌──────────────────────────┐             ┌────────────────────┐
│  后端 HTTP API (node:http) │ ─────────▶ │  前端 SPA (Vite+React) │
└──────────────────────────┘   JSON       └────────────────────┘
```

- 后端不依赖 Web 框架，手写极简路由（`src/server/http.js`、`src/routes/api.js`）。
- DuckDB 访问集中在单一 repository 层（`src/db/duckdb.js`），上层不直接依赖绑定。
- 前端只用 `fetch` 调后端 API，**不直接访问 C++ 或 CSV 文件**。

---

## 3. 测试场景的组织与创建

**场景 = 目录**。每个测试场景是一个自包含目录，包含本次计算所需的全部数据集 CSV。
`ipc-core-benchmark/data/demo/` 下预置三个基准场景：

| 场景目录 | 覆盖引擎 | 基准结论 |
| :--- | :--- | :--- |
| `delivery-benchmark/` | delivery | 需求 30 件 @ Day3 → 承诺 Day 3、预留 3 工时 |
| `itp-iop-benchmark/` | itp, iop | 下派 3 / 阻断 1 |
| `substitution-benchmark/` | substitution | 三类分配 10、安全库存 10 未侵占 |

### 七类数据集列契约

| 文件 | 用于引擎 | 列 |
| :--- | :--- | :--- |
| `parts.csv` | 全部 | `part_id,part_code,site,safety_stock,initial_on_hand,lot_size,lead_time` |
| `bom.csv` | delivery, substitution | `parent_id,child_id,usage_qty,alt_class,alt_group,target_ratio,historical_qty,lot_size` |
| `atp_supply.csv` | delivery | `supply_code,supply_type,part_id,available_day,qty,priority` |
| `capacity.csv` | delivery | `work_center,day,capacity_hours` |
| `demands_master.csv` | itp | `demand_id,part_id,due_day,qty,priority,customer_group,region` |
| `demands_execution.csv` | iop | 同上 |
| `substitution_group.csv` | substitution | `alt_group,alt_class,member_part_id,target_ratio,historical_qty` |

引擎 → 所需数据集的映射（用于可运行性判定）：

- `delivery`：`parts` + `bom` + `atp_supply` + `capacity`
- `itp`：`parts` + `demands_master`
- `iop`：`parts` + `demands_master` + `demands_execution`（CLI 内部先生成 ITP 配额再跑 IOP）
- `substitution`：`parts` + `bom` + `substitution_group`

### 新建场景

内置场景根（`ipc-core-benchmark/data/`）为**只读**；新建场景写入可写场景根
（默认 `ipc-demo/scenarios/`），不会改动仓库内既有数据：

```bash
# 创建一个空场景目录
curl -X POST http://127.0.0.1:3001/api/scenarios \
     -H 'content-type: application/json' -d '{"scenarioId":"my-scenario"}'

# 逐个提供数据集 CSV（提供后立即导入 DuckDB）
curl -X PUT http://127.0.0.1:3001/api/scenarios/my-scenario/datasets/parts \
     -H 'content-type: text/csv' --data-binary @parts.csv
```

---

## 4. 数据导入

- **启动导入（默认）**：`npm start` / `npm run demo` 会发现全部场景并幂等导入输入数据
  （按 `scenario_id` 先删后插，重复导入不产生重复行）。
- **全量/大数据集导入**：`npm run import:full [-- <scenarioId>]`，导入指定或全部场景并报告
  耗时与总行数；交互式求解默认面向样例/中等规模，全量导入与启动路径解耦。
- **运行输出导入**：每次引擎运行的输出 CSV 在运行完成后自动导入，按 `run_id` 归属；
  输入与输出 CSV 原文同时归档到 `run_artifacts` 供审计与重新导入。

DuckDB 位置默认 `ipc-demo/.data/ipc_demo.duckdb`（可用 `DUCKDB_PATH` 覆盖）。

---

## 5. 配置项（环境变量）

| 变量 | 默认值 | 说明 |
| :--- | :--- | :--- |
| `PORT` | `3001` | 后端监听端口 |
| `HOST` | `127.0.0.1` | 监听地址，**默认仅回环** |
| `DUCKDB_PATH` | `ipc-demo/.data/ipc_demo.duckdb` | DuckDB 单文件数据库 |
| `SCENARIO_ROOT` | `ipc-core-benchmark/data` | 只读内置场景根 |
| `WRITABLE_SCENARIO_ROOT` | `ipc-demo/scenarios` | 新建场景落点 |
| `ENGINE_BIN` | `ipc-core-benchmark/bin/ipc_engine_cli` | C++ CLI 可执行文件 |
| `ENGINE_TIMEOUT_MS` | `120000` | 子进程运行时限（毫秒） |
| `RUNS_DIR` | `ipc-demo/runs` | 每次运行的工作目录根 |
| `MAX_CONCURRENT_RUNS` | `2` | 并发子进程上限 |
| `PUBLIC_DIR` | `ipc-demo/public` | 前端静态资源目录 |

---

## 6. C++ CLI 契约

完整契约见 [`ipc-core-benchmark/cli/README.md`](../ipc-core-benchmark/cli/README.md)。

```bash
ipc_engine_cli --engine <delivery|itp|iop|substitution> --in <场景目录> --out <输出目录> [选项]
```

- 场景目录可直接作为 `--in`，无需格式转换。
- 退出码：`0` 成功；`2` 参数错误；`3` 输入缺失/不可读；`4` 计算或写出失败。
- 约束键 `day/family_id/cust_group_id/region_id` **只在 C++ 侧计算**（`std::hash` 属实现定义），
  Node 与前端不重算。

直接调用示例：

```bash
mkdir -p /tmp/out
../ipc-core-benchmark/bin/ipc_engine_cli --engine delivery \
    --in ../ipc-core-benchmark/data/demo/delivery-benchmark --out /tmp/out \
    --part-id 0 --due-day 3 --qty 30
cat /tmp/out/delivery_result.csv
```

---

## 7. 后端 API 契约

Base URL：`http://127.0.0.1:3001`。前端所有数据均经此 API 获取。

### 健康与场景

| 方法 | 路径 | 说明 |
| :--- | :--- | :--- |
| `GET` | `/api/health` | 分别标明 DuckDB / 场景 / C++ 可执行文件是否就绪 |
| `GET` | `/api/scenarios` | 场景列表与数据集概览（含各引擎可运行性） |
| `GET` | `/api/scenarios/:id` | 场景详情 + 导入记录 |
| `POST` | `/api/scenarios` | 新建场景，body `{scenarioId}` |
| `PUT` | `/api/scenarios/:id/datasets/:key` | 提供/替换某数据集 CSV 并导入 |
| `POST` | `/api/scenarios/:id/import` | 幂等重新导入该场景输入数据 |

### 输入浏览

| 方法 | 路径 | 说明 |
| :--- | :--- | :--- |
| `GET` | `/api/scenarios/:id/inputs` | 按业务类别（需求/供给/资源/计划供给/物料主数据/产品结构/替代料组）返回数据集与行数 |
| `GET` | `/api/scenarios/:id/inputs/:key?limit=&offset=` | 某输入数据集明细（来自 DuckDB） |
| `GET` | `/api/scenarios/:id/raw/:filename` | 原始 CSV 文本（与磁盘文件逐字一致） |

### 引擎运行

| 方法 | 路径 | 说明 |
| :--- | :--- | :--- |
| `POST` | `/api/runs/delivery` | body `{scenarioId, part_id?, due_day?, qty?, priority?}` |
| `POST` | `/api/runs/itp` | body `{scenarioId, buffer_factor?}` |
| `POST` | `/api/runs/iop` | body `{scenarioId}` |
| `POST` | `/api/runs/substitution` | body `{scenarioId, net_demand?, alt_class?, alt_group?, day?, parent_id?}` |
| `POST` | `/api/runs` | 通用入口，body 含 `engineType` |

响应 `201`，包含 `runId`、`durationMs`（端到端）、`engineDurationMs`（C++ 自报）、
`outputs`（各输出数据集行数）与 `conclusion`（业务结论）。错误语义：

| 状态 | `kind` | 含义 |
| :--- | :--- | :--- |
| `400` | `invalid_request` | 参数非法（不启动子进程） |
| `422` | `scenario_incomplete` | 场景缺少该引擎所需数据集 |
| `503` | `engine_missing` | C++ 可执行文件缺失/不可执行 |
| `504` | `engine_timeout` | 子进程超时 |
| `502` | `engine_nonzero_exit` | 子进程非零退出 |
| `502` | `malformed_output` | 输出 CSV 缺失或列不全 |

> 业务失败（如替代料不可分配，退出码 0、0 分配）返回 `201` + 业务结论，区别于编排失败。

### 历史、输出与透视

| 方法 | 路径 | 说明 |
| :--- | :--- | :--- |
| `GET` | `/api/runs?scenarioId=&engineType=&limit=` | 运行历史（按创建时间倒序） |
| `GET` | `/api/runs/:runId` | 运行详情（含输出数据集行数与产物列表） |
| `GET` | `/api/runs/:runId/outputs/:key?limit=` | 某输出数据集明细（按 `run_id`，无重算） |
| `GET` | `/api/runs/:runId/artifacts/:direction/:filename` | 运行输入/输出 CSV 原文 |
| `GET` | `/api/scenarios/:id/pivot?runId=&metrics=&dimension=&dimValue=` | 计划透视（行=指标×维度，列=天，四族） |
| `GET` | `/api/scenarios/:id/pivot/range` | 时间范围 |
| `GET` | `/api/scenarios/:id/pivot/cell?metric=&day=&dimValue=&runId=` | 单元格下钻明细 |

计划透视响应结构：`{ scenarioId, runId, days[], lo, hi, families[], metrics[], rows[] }`，
其中 `rows[]` 每行含 `{ metric, label, family, unit, dimension, dimValue, values[], hasData,
negativeIsOverload, error }`。**`values` 中 `null` 表示无计划（空）**，与数值 `0` 区分；
`negativeIsOverload` 为真的指标在值为负时前端高亮（如剩余产能为负 = 超载）。

示例：

```bash
curl 'http://127.0.0.1:3001/api/scenarios/delivery-benchmark/pivot?dimension=work_center&dimValue=WC_01'
curl -X POST http://127.0.0.1:3001/api/runs/delivery \
     -H 'content-type: application/json' \
     -d '{"scenarioId":"delivery-benchmark","due_day":3,"qty":30}'
```

---

## 8. 前端视图

| 视图 | 关键能力 |
| :--- | :--- |
| 场景浏览 | 按业务类别查看输入数据、原始 CSV、场景内历次运行输出、缺失/不可运行提示 |
| 计划透视 | 行=指标×维度、列=天，四族分组可折叠，筛选按天重算，空/零区分，超载/缺口高亮，单元格下钻 |
| 交付承诺 | BOM 层级、库存/产能水位、请求 vs 承诺时间轴、缺口与回滚标识、逐步回放 |
| ITP / IOP | 四维约束键配额 vs 消耗、临近/达到上限区分、阻断订单列表与聚合指标 |
| 替代料 | 类别配额/历史量/本次分配、安全库存保护带、选中成员高亮与依据 |
| 性能与历史 | 端到端 vs C++ 自报耗时、处理条数、吞吐量、历史对比图表、点击加载 `run_id` |

---

## 9. 测试

```bash
cd ipc-demo
npm test                       # 场景管理 / 持久化 / 透视一致性 / API 集成 / 前端组件 全部单测
../ipc-core-benchmark/cli/cli_regression.sh   # C++ CLI 四引擎回归（33 断言）
```

`npm test` 使用隔离的临时场景根与临时 DuckDB，不会写入仓库 `data/` 目录。
