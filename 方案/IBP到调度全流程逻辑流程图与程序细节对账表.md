# IPC 核心专栏：从 IBP 运营计划到车间排产执行计划的全景持久化 DB 表结构（199 表全景）与底层算法双层解构表

> **系统工程导言与双引擎架构物理学**
> 本文档为 IPC (Integrated Planning & Control) 交付大脑的**全景持久化数据库 199 张物理表全集、运营与执行计划算法及工程实现主对账表**。
> 
> **双引擎协同架构物理学**：
> 1. **感知、整合与持久化数据库引擎层 (Perception, Integration & Persistence DB Layer - DuckDB, 199 张物理表)**：
>    数据库本身即是 IPC 引擎不可分割的核心物理组成部分。在系统感知外围 ERP / MES / WMS / SCM / CRM 等业务系统数据后，通过 DuckDB 引擎实施数据归一化整合、3D 需求张量构建、IBP 预测共识融合、MEIO 安全库存水线求解、配额屏障落库及全网 Pegging 查账。
> 2. **高性能 C++ DOD 裸金属内存计算引擎层 (High-Performance C++ DOD In-Memory Calculation Engine)**：
>    将数据库中的核心物理表（如物料、BOM 拓扑、在库水线、在途供应、战术配额、车间能力等）一次性编译装载进 64 字节 Cache-Line 严格物理对齐的连续内存数组中（如 `std::vector<PartSiteRecord>`, `FlatBomItem`, `Axis`, `AllotmentConstraintKeyHash`, `OptimisticInventoryPool`），实现百万级节点规模下的毫秒级 CTP 探路、有限能力 MRP 消纳与 DBD 车间派程。

---

## 一、 Master Logic Flowchart (全景 Visio 规范逻辑流程图)

![IPC 从 IBP 到车间排产调度全景 Visio 规范逻辑流程图](images/ibp_to_scheduling_master_flowchart.png)
*图 1-1 IPC 从战略 IBP 控制塔至微观车间派工排产调度的 Visio 规范主逻辑流程与 C++ 引擎架构图*

```mermaid
flowchart TD
    %% 样式定义
    classDef startEnd fill:#1f2937,stroke:#9ca3af,stroke-width:2px,color:#fff;
    classDef processStyle fill:#1e3a8a,stroke:#60a5fa,stroke-width:2px,color:#fff;
    classDef parallelStyle fill:#581c87,stroke:#c084fc,stroke-width:2px,color:#fff;
    classDef decisionStyle fill:#7c2d12,stroke:#fb923c,stroke-width:2px,color:#fff;
    classDef outputStyle fill:#065f46,stroke:#34d399,stroke-width:2px,color:#fff;
    classDef dsStyle fill:#111827,stroke:#38bdf8,stroke-width:1.5px,color:#38bdf8;

    %% 阶段 0: 系统点火与起点双轨并发
    Start([系统点火: 载入S&OP预测与客户订单]) :::startEnd --> Stage0_Fork{Fork-Join 起点并发分流} :::parallelStyle

    subgraph Phase0 ["【阶段 0】起点双轨 Fork-Join 并发编译"]
        Stage0_Fork --> TaskA["并行轨 A: 订单优先级排序 & 维度绑定<br/>(encode_composite_priority 位打包排序)"] :::parallelStyle
        Stage0_Fork --> TaskB["并行轨 B: 价值链网络拓扑展开 & 低层码编译<br/>(compile_low_level_codes / LLC 拓扑排序)"] :::parallelStyle
        TaskA --> Stage0_Join[Fork-Join 双轨收敛屏障] :::parallelStyle
        TaskB --> Stage0_Join
    end

    %% 阶段 1: IBP 战略控制塔与财业双生 (运营计划)
    Stage0_Join --> Phase1_IBP["【阶段 1】IBP 财业双生双层规划 (Bilevel Optimization)"] :::processStyle
    
    subgraph Phase1 ["【运营计划】IBP 战略与 MEIO 多阶库存优化"]
        Phase1_IBP --> IBP_Consensus["3D 共识预测融合<br/>(Sales + Marketing + Statistical + Strategic)"] :::processStyle
        IBP_Consensus --> IBP_HW["Holt-Winters 时序预测 & 比例拆解算子<br/>(Proportional Disaggregation)"] :::processStyle
        IBP_HW --> IBP_MEIO["MEIO 多阶库存优化与安全库存算子<br/>(parts_daily_demand 展平数组 & Z-Score 方差传递)"] :::processStyle
    end

    %% 阶段 2: ITP 战术冲销 (运营计划)
    IBP_MEIO --> Phase2_Tensor["【阶段 2】构建 3D DOD 需求张量<br/>gross_demand[part_id][day][dim]"] :::processStyle

    subgraph Phase2 ["【运营计划】ITP 战术净需求冲销"]
        Phase2_Tensor --> Forecast_Consume["Forecast Consumption 预测冲销滑动窗口<br/>Unconsumed = max(0, Forecast - sum(Orders))"] :::processStyle
        Forecast_Consume --> Net_Demand["输出纯净 Net Demand Stream (净需求流)"] :::outputStyle
    end

    %% 阶段 3: ITP 战术配额 5 大条件门控 (运营计划)
    Net_Demand --> Gate1{"【判决门控 1】Gating 瓶颈判定<br/>Demand Qty > Network Capacity?"} :::decisionStyle

    subgraph Phase3 ["【运营计划】ITP 战术配额条件判决树"]
        Gate1 -- "No (无瓶颈)" --> Gate1_FIFS["FIFS (先到先得) 顺畅释放配额"] :::processStyle
        Gate1 -- "Yes (触发瓶颈)" --> Gate2{"【判决门控 2】时间桶属性判定<br/>Time Horizon Window?"} :::decisionStyle

        Gate2 -- "短期 1-4 周" --> Gate2_Priority["Strategic Priority 算子<br/>High 承诺 100% 划拨配额<br/>剩余 Supply 注入 Surplus 池"] :::processStyle
        Gate2 -- "长期 5 周+" --> Gate2_FairShare["FairShare 比例分摊算子<br/>Assigned = Supply * (Demand / Total_Demand)"] :::processStyle

        Gate2_Priority --> Gate3{"【判决门控 3】跨时间桶 Netting 判定<br/>Shortage(t) > 0 AND Surplus(t+k) > 0?"} :::decisionStyle
        Gate2_FairShare --> Gate3

        Gate3 -- "Yes" --> Gate3_LeftShift["Left-Shifting 向左切片拉动算子<br/>Pull Qty = min(Shortage(t), Surplus(t+k))"] :::processStyle
        Gate3 -- "No" --> Gate3_Pass["保持当前时间桶配额分配"] :::processStyle

        Gate1_FIFS --> Allotment_Cap["生成 ITP 刚性配额防波堤护栏<br/>ipc_allotment_constraint (AllotmentConstraintKeyHash 哈希Fast-Path)"] :::outputStyle
        Gate3_LeftShift --> Allotment_Cap
        Gate3_Pass --> Allotment_Cap
    end

    %% 阶段 4: LBL MRP 拓扑净额 (执行计划)
    Allotment_Cap --> Phase4_MRP["【阶段 4】LBL MRP 逐层消纳点火 (Level 0 -> Level N)"] :::processStyle

    subgraph Phase4 ["【执行计划】LBL MRP & 替换料分配"]
        Phase4_MRP --> MRP_OpenMP["OpenMP 多线程同层并行消纳<br/>(#pragma omp parallel for)"] :::parallelStyle
        MRP_OpenMP --> MRP_Atomic["std::atomic 无锁优先级争用<br/>(compare_exchange_weak)"] :::parallelStyle
        MRP_Atomic --> Gate_Alt{"主料库存/在途齐套判定<br/>Main Component Available?"} :::decisionStyle

        Gate_Alt -- "Yes" --> MRP_Main["主料直接扣减 & 生成 Planned Order"] :::processStyle
        Gate_Alt -- "No" --> MRP_Swap["Alternate Substitution 替换料分配算子<br/>扫描 Alt Group 优先级并扣减"] :::processStyle

        MRP_Main --> MRP_Output["生成 Planned Orders & Pegging 扣减账本"] :::outputStyle
        MRP_Swap --> MRP_Output
    end

    %% 阶段 5: IOP 微观派发与 4 步资源置换 (执行计划)
    MRP_Output --> Gate4{"【判决门控 4】IOP 交期契约定锚判定<br/>Due Date > Best Can Do?"} :::decisionStyle

    subgraph Phase5 ["【执行计划】IOP 微观派发与资源置换闭环"]
        Gate4 -- "Yes" --> Gate4_PSD1["锚定交期契约红线 PSD = Due Date"] :::processStyle
        Gate4 -- "No" --> Gate4_PSD2["锚定交期契约红线 PSD = Best Can Do"] :::processStyle

        Gate4_PSD1 --> Gate5{"【判决门控 5】微观扰动监测<br/>Micro-Disturbance Detected?"} :::decisionStyle
        Gate4_PSD2 --> Gate5

        Gate5 -- "No" --> IOP_Pass["维持原定排程计划"] :::processStyle
        Gate5 -- "Yes (触发高优先插单/断供)" --> Step4_Swap["Resource Swapping 4 步置换算子<br/>1. 高优先插单判定<br/>2. 全网软预留扫描<br/>3. 低优先资源剥离 (De-allocation)<br/>4. 被置换任务二次排期"] :::processStyle

        IOP_Pass --> IOP_CapGuard["强制校验 Allotment Cap 护栏"] :::processStyle
        Step4_Swap --> IOP_CapGuard
    end

    %% 阶段 6: MCDS / DBD 车间调度与工单刚性反写 (执行计划)
    IOP_CapGuard --> Phase6_Scheduling["【阶段 6】MCDS / DBD 微观派程调度引擎<br/>(ipc::run_dbd_dispatch_engine)"] :::processStyle

    subgraph Phase6 ["【执行计划】车间调度与闭环反写"]
        Phase6_Scheduling --> Sched_Capacity["车间工位/机台产能匹配 (wc_daily_capacity)"] :::processStyle
        Sched_Capacity --> Sched_DAG["工单 DAG 依赖生成与开工/完工时刻求解"] :::processStyle
        Sched_DAG --> Bilevel_Backwrite["闭环 MPC 180 秒刚性反写引擎<br/>DuckDB Appender 流式注入 & 派工单 Work Order 反写 MES/WMS"] :::outputStyle
    end

    Bilevel_Backwrite --> End([排产结束: ROIC 逆势提升 1.8% / 财业无损闭环]) :::startEnd
```

---

## 二、 全量持久化 DB 物理表结构与数据字典 (199 表全景解构)

IPC 持久化数据库共收录 **199 张物理表**，详细记载了从外围感知清洗、主数据映射、预测共识到 C++ 内存计算及最终排产落库的全过程。全表按 6 大业务功能模块归档：

```
+------------------------------------------------------------------------------------+
|                IPC 全量 199 张物理数据库表功能模块归档架构                           |
+------------------------------------------------------------------------------------+
| 1. 核心计划与排产 (Core Planning & Execution): 包含 10 张表 (C++ 内存加载)          |
| 2. 联副产品分级优化 (Co-product Optimization): 包含 8 张表 (分级切片)               |
| 3. ETO 协同项目管理 (ETO Project & WBS): 包含 6 张表 (CPM 关键路径)                 |
| 4. IBP 财务与预测共识 (IBP Planning & Consensus): 包含 30 张表 (财业双生)           |
| 5. IO 安全库存水位优化 (IO Safety Stock Policy): 包含 12 张表 (MEIO 方差传递)        |
| 6. 基础支撑、感知整合与主数据 (Master Data & Perception): 包含 133 张表 (全网感知)   |
+------------------------------------------------------------------------------------+
```

完整 199 表物理列字段与 ER 图明细已同步归档于独立全景数据字典：📄 **[ipc_data_dictionary.md](file:///h:/IPC/docs/ipc_data_dictionary.md)**（9,667 行）。

---

### 2.1 核心主数据与静态拓扑表 (Core Master Data & Topology)

#### 1. `ipc_part` / `ipc_material_node` (物料主数据表)
> **业务说明**：定义全局物料的物理编码、分类（成品/半成品/原材料/替代料）、计量单位、平均销售单价及安全库存估值。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `part` | 物料唯一编码 | `VARCHAR(100)` | 🔑 **PK / NOT NULL** | 零部件全局唯一识别码 (SKU Code) | `PartSiteRecord.part_code` |
| `part_type` | 物料分类 | `VARCHAR(50)` | Nullable | 类别：`FINISHED` (成品), `SEMI` (半成品), `RAW` (原料), `ALT` (替代料) | `PartSiteRecord.part_type` |
| `mrp_rule` | MRP 计算规则 | `VARCHAR(20)` | Nullable | 消纳控制逻辑，如 `LBL_NETTING`, `REORDER_POINT` | `PartSiteRecord.mrp_rule` |
| `site` | 归属工厂/站点 | `VARCHAR(8)` | 🔑 **PK / NOT NULL** | 工厂或仓库站点编码 (Site Code，对应 SAP Plant) | `PartSiteRecord.site` |
| `is_phantom` | 虚拟件标识 | `BOOLEAN` | Default `FALSE` | `TRUE` 表示虚拟件，MRP 展开时直接穿透跳过工单生成 | `PartSiteRecord.is_phantom` |
| `selling_ave_price`| 平均销售单价 | `DOUBLE` | Default `0.0` | 财务结算及营业收入折算的基准平均售价 | `PartSiteRecord.cost` |
| `transshipment_cost`| 单件调拨成本 | `DOUBLE` | Default `0.0` | 厂区间跨站点物流调拨的单件运费成本 | `PartSiteRecord.transshipment_cost` |
| `transshipment_lead_time`| 调拨提前期 | `INTEGER` | Default `0` | 厂区间跨站点调拨的物理周期 (天数) | `PartSiteRecord.transshipment_lead_time` |
| `lead_time` | 固定制造提前期 | `DOUBLE` | Default `0.0` | 基础生产工时提前期 | `PartSiteRecord.lead_time` |
| `safety_stock` | 安全库存目标量 | `DOUBLE` | Default `0.0` | 根据 MEIO 算法算出的安全水线水位 | `PartSiteRecord.safety_stock` |
| `safety_stock_value`| 安全库存资金占用| `DOUBLE` | `GENERATED` | 计算列：`safety_stock * selling_ave_price` | 财务资产负债表审计项 |

#### 2. `ipc_bom_route` (工艺路线与 BOM 绑定表)
> **业务说明**：定义物料在特定站点下的工艺 Route 与 BOM ID 映射，控制 CTP 多路径选择。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `site` | 站点编码 | `VARCHAR(8)` | 🔑 **PK / NOT NULL** | 工厂站点编码 | `SourceConstraintRecord.site` |
| `part` | 物料编码 | `VARCHAR(40)` | 🔑 **PK / NOT NULL** | 父件物料编码 | `SourceConstraintRecord.part_id` |
| `bomid` | BOM 结构编号 | `VARCHAR(40)` | 🔑 **PK / NOT NULL** | 绑定的 BOM 版本唯一标识 | `FlatBomItem.parent_id` |
| `priority` | 路线优先级 | `INTEGER` | Default `1` | 优先推荐顺序，数值越小越优先 | `AlternativeRouting.priority` |
| `bom_type` | BOM 业务类型 | `VARCHAR(10)` | Nullable | 制造 BOM (`M-BOM`), 研发 BOM (`R-BOM`) | `SourceConstraintRecord.bom_type` |

#### 3. `ipc_bom_item` (BOM 明细与动态替代配置表)
> **业务说明**：记录组装件与子件的拓扑展开关系，包含替代组、优先级、Lot-size 包装规格及有效性窗口。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `bomid` | BOM 结构编号 | `VARCHAR(40)` | NOT NULL | 关联 `ipc_bom_route.bomid` | `FlatBomItem.parent_id` |
| `site` | 站点编码 | `VARCHAR(8)` | NOT NULL | 站点编码 | `FlatBomItem.site` |
| `component` | 子件物料编码 | `VARCHAR(40)` | NOT NULL | 子组件唯一物料编码 | `FlatBomItem.child_id` |
| `perqty` | 单件消耗用量 | `DOUBLE` | Default `1.0` | 制造单位父件所需的子件基准单耗 ($perqty$) | `FlatBomItem.per_qty` |
| `scrap` | 制造损耗率 | `DOUBLE` | Default `0.0` | 生产过程中的物料损耗比例 (0.05 代表 5% 损耗) | `FlatBomItem.scrap` |
| `alt_grp` | 替代组编码 | `VARCHAR(10)` | Nullable | 相同替代组内的物料具备功能互换性 | `FlatBomItem.alt_group_id` |
| `priority` | 替代优先级 | `INTEGER` | Default `0` | 组内替代顺序，值越小越优先使用 | `FlatBomItem.alt_priority` |
| `target` | 目标分配比例 | `DOUBLE` | Default `1.0` | 多路并行分摊时的目标比例系数 ($target$) | `FlatBomItem.target_ratio` |
| `lot_size` | 包装批次规格 | `DOUBLE` | Default `0.0` | 向上舍入批次规格 ($LotSize$) | `FlatBomItem.lot_size` |
| `eff_start_day` | 生效起始天数 | `INTEGER` | Default `-1` | 相对 Rundate 的生效起始日 (ECN 控期) | `FlatBomItem.eff_start_day` |
| `eff_end_day` | 生效失效天数 | `INTEGER` | Default `-1` | 相对 Rundate 的失效终止日 (ECN 软切) | `FlatBomItem.eff_end_day` |
| `ltb_limit` | 终期买入上限 | `DOUBLE` | Default `-1.0`| LTB (Life-Time Buy) 备件最大累计消耗配额 | `FlatBomItem.ltb_limit` |

---

### 2.2 运营计划表 (Operational Plan Tables: S&OP, IBP & ITP)

#### 4. `ipc_independent_demand` (独立需求与客户订单表)
> **业务说明**：存储来自 S&OP 的预测需求以及真实客户销售订单，包含位打包优先级所需的所有维度。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `demand` | 需求订单编号 | `VARCHAR(50)` | 🔑 **PK / NOT NULL** | 独立需求或销售订单号 (SO Number) | `IndependentDemand.demand_id` |
| `item` | 订单行号 | `DOUBLE` | 🔑 **PK / NOT NULL** | 订单明细行号 | `IndependentDemand.item_id` |
| `part` | 需求物料编码 | `VARCHAR(40)` | NOT NULL | 成品物料编码 | `IndependentDemand.part_id` |
| `customer` | 客户编码 | `VARCHAR(40)` | Nullable | 客户唯一标识，关联客户层级 | `IndependentDemand.customer` |
| `request_due_date` | 客户期望交付日 | `DATE` | NOT NULL | 契约交付日期 (Request Due Date) | `IndependentDemand.due_day` |
| `request_qty` | 原始请求数量 | `DOUBLE` | Default `0.0` | 客户下单原始数量 | `IndependentDemand.qty` |
| `open_qty` | 未履约开口数量 | `DOUBLE` | Default `0.0` | 尚未通过 ATP/CTP 锁定交付的剩余缺口 | `IndependentDemand.open_qty` |
| `order_priority` | 原始优先级 | `INTEGER` | Default `99` | 原始业务优先级数值 | `IndependentDemand.original_priority` |
| `customer_tier` | 客户战略等级 | `INTEGER` | Default `3` | 1-Tier1 (战略), 2-Tier2 (核心), 3-Tier3 (普通)| `IndependentDemand.customer_tier` |
| `revenue` | 订单营业收入额 | `DOUBLE` | Default `0.0` | 该笔订单对应的合同销售总金额 (元) | `IndependentDemand.revenue` |
| `dimension_grp` | 特征维度组 | `VARCHAR(10)` | Nullable | Semiconductor Binning 等多维消纳标签 | `IndependentDemand.dimension_val` |

#### 5. `ipc_allotment_constraint` (ITP 战术配额防波堤约束表)
> **业务说明**：保存 ITP 阶段通过 5 大门控树生成的战术配额额度护栏，作为 IOP 运行时的刚性屏障。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `scenario_id` | 方案场景 ID | `VARCHAR(32)` | 🔑 **PK / NOT NULL** | 沙盒场景标识 (如 `BASE`, `PESSIMISTIC`) | `AllotmentConstraintKey.scenario` |
| `part_code` | 物料编码 | `VARCHAR(40)` | 🔑 **PK / NOT NULL** | 配额管控物料号 | `AllotmentConstraintKey.part_id` |
| `site_code` | 工厂站点 | `VARCHAR(8)` | 🔑 **PK / NOT NULL** | 供应厂区 | `AllotmentConstraintKey.site_id` |
| `region` | 销售大区 | `VARCHAR(20)` | Default `'*'` | 配额划拨地理大区 (如 `APAC`, `NA`) | `AllotmentConstraintKey.region_id` |
| `customer_group` | 客户集群 | `VARCHAR(20)` | Default `'*'` | 配额划拨客户群 (如 `KEY_ACCOUNTS`) | `AllotmentConstraintKey.cust_group_id` |
| `product_family` | 产品系列 | `VARCHAR(40)` | Default `'*'` | 归属产品线 | `AllotmentConstraintKey.family_id` |
| `day` | 时间桶天数 | `INTEGER` | 🔑 **PK / NOT NULL** | 相对 Rundate 的天数索引 | `AllotmentConstraintKey.day` |
| `itp_calculated_qty`| ITP 求解配额 | `DOUBLE` | Default `0.0` | 算法自动计算出的建议配额上限 | `AllotmentValue.calculated_limit` |
| `override_qty` | 人工干预调整量| `DOUBLE` | Default `0.0` | 计划员手工修正配额量 | `AllotmentValue.override_limit` |
| `is_locked` | 锁定配额标识 | `BOOLEAN` | Default `FALSE` | `TRUE` 表示锁定配额，算法不得自动修改 | `AllotmentValue.is_locked` |

#### 6. `ipc_allotment_ledger` (ITP 配额实时消耗账本表)
> **业务说明**：实时追踪 IOP 执行过程中对 ITP 配额的扣减情况与剩余防波堤水位。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `part_code` | 物料编码 | `VARCHAR(40)` | NOT NULL | 物料编码 | `AllotmentLedger.part_id` |
| `day` | 时间桶天数 | `INTEGER` | NOT NULL | 时间天数 | `AllotmentLedger.day` |
| `allotment_limit` | 刚性上限额度 | `DOUBLE` | Default `0.0` | 允许消纳的最大上限配额 | `AllotmentLedger.limit_qty` |
| `consumed_qty` | 已扣减消耗量 | `DOUBLE` | Default `0.0` | 被高优先级订单占用的额度 | `AllotmentLedger.consumed_qty` |
| `available_qty` | 剩余可用额度 | `DOUBLE` | Default `0.0` | `allotment_limit - consumed_qty` | `AllotmentLedger.available_qty` |
| `blocked_demand_qty`| 被拦截需求量 | `DOUBLE` | Default `0.0` | 因超出 Allotment 护栏而被截留的超额需求 | `AllotmentLedger.blocked_qty` |

---

### 2.3 执行计划表 (Execution Plan Tables: IOP, MRP & DBD)

#### 7. `ipc_onhand` (物理在库库存表)
> **业务说明**：记录各库位、物料在特定可用日期下的物理真实库存量，作为 LBL-MRP 扣减的起点水位。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `location` | 物理库位编码 | `VARCHAR(10)` | 🔑 **PK / NOT NULL** | 仓库物理网格/库位号 | `Axis.locations` |
| `part` | 物料编码 | `VARCHAR(40)` | 🔑 **PK / NOT NULL** | 物料唯一编码 | `Axis.part_id` |
| `site` | 工厂站点 | `VARCHAR(8)` | 🔑 **PK / NOT NULL** | 物理厂区 | `Axis.site_id` |
| `available_date` | 可用起始日期 | `DATE` | 🔑 **PK / NOT NULL** | 可用于 ATP/CTP 扣减的就绪日期 | `Axis.dates` |
| `qty` | 物理库存数量 | `DOUBLE` | Default `0.0` | 现有物理库存数量 | `Axis.qtys` / `OptimisticInventoryPool`|
| `inventory_type` | 库存质量类型 | `VARCHAR(10)` | Default `'Unrestricted'`| 状态：`Unrestricted` (无限制), `Quality` (质检中) | `Axis.types` |

#### 8. `ipc_scheduled_receipt` (在途供应订单表)
> **业务说明**：存储已发放但尚未到货的采购单 (PO) 与在工工单 (WO) 资源。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `sr_id` | 在途单据编号 | `VARCHAR(18)` | 🔑 **PK / NOT NULL** | 采购订单号或生产工单号 | `ScheduledReceipt.sr_id` |
| `to_part` | 交付物料编码 | `VARCHAR(40)` | NOT NULL | 预计入库的物料号 | `ScheduledReceipt.part_id` |
| `to_site` | 接收厂区站点 | `VARCHAR(8)` | NOT NULL | 目的站点 | `ScheduledReceipt.site_id` |
| `qty` | 在途到货数量 | `DOUBLE` | Default `0.0` | 确定到货的物理数量 | `ScheduledReceipt.qty` |
| `request_due_date` | 承诺到货日期 | `DATE` | NOT NULL | 供应商/车间承诺齐套日期 | `ScheduledReceipt.due_day` |
| `supply_status` | 在途执行状态 | `VARCHAR(10)` | Default `'RELEASED'`| `RELEASED` (已下达), `IN_TRANSIT` (运输中) | `ScheduledReceipt.status` |

#### 9. `ipc_planned_order` (建议计划订单表)
> **业务说明**：LBL-MRP 算法求解后生成的制造工单 (Make)、采购计划 (Buy) 与调拨单 (Transfer)。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `ipc_planned_order`| 计划订单编号 | `VARCHAR(18)` | 🔑 **PK / NOT NULL** | 系统自动生成的工单唯一流水号 | `PlannedOrder.order_id` |
| `part` | 制造/采购物料 | `VARCHAR(40)` | 🔑 **PK / NOT NULL** | 目标补货物料号 | `PlannedOrder.part_id` |
| `site` | 生产/接收站点 | `VARCHAR(8)` | 🔑 **PK / NOT NULL** | 生产或接收厂区 | `PlannedOrder.site_id` |
| `request_start_date`| 建议开工日期 | `DATE` | NOT NULL | 考虑制造提前期 $LT$ 前推计算得到的开工期 | `PlannedOrder.start_day` |
| `due_date` | 建议完工日期 | `DATE` | NOT NULL | 承诺交付或就绪日期 | `PlannedOrder.finish_day` |
| `qty` | 建议计划数量 | `DOUBLE` | Default `0.0` | 原始计算出的补货需求量 | `PlannedOrder.qty` |
| `eff_qty` | 实际有效数量 | `DOUBLE` | Default `0.0` | 考虑损耗率 $scrap$ 后的实际投放量 | `PlannedOrder.eff_qty` |
| `dimension_grp` | 派生特征维度 | `VARCHAR(10)` | Nullable | 继承自顶层订单的特征维度值 | `PlannedOrder.dimension_val` |
| `source` | 供应来源类型 | `VARCHAR(10)` | Default `'MAKE'` | `MAKE` (自制), `BUY` (外购), `TRANSFER` (调拨) | `PlannedOrder.source` |

#### 10. `ipc_planned_supply_assignment` (计划钉结分配账本表 - Planned Pegging)
> **业务说明**：溯源记录独立需求与计划订单/库存/在途之间的多对多动态钉结关系（Pegging）。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `demand` | 独立需求编号 | `VARCHAR(18)` | 🔑 **PK / NOT NULL** | 关联顶层需求订单号 | `PeggingRecord.demand_id` |
| `item` | 需求明细行号 | `DOUBLE` | 🔑 **PK / NOT NULL** | 关联需求行号 | `PeggingRecord.item_id` |
| `assigned_part` | 实际分配物料号 | `VARCHAR(40)` | NOT NULL | 实际被消耗的物料（若替代则为替代件号） | `PeggingRecord.part_id` |
| `assigned_qty` | 锁定钉结数量 | `DOUBLE` | Default `0.0` | 绑定锁定的物理数量 | `PeggingRecord.qty` |
| `available_date` | 齐套交付日期 | `DATE` | NOT NULL | 实际满足需求的 ATP 交付日期 | `PeggingRecord.day` |
| `planned_order` | 关联计划订单号 | `VARCHAR(18)` | Nullable | 绑定的 `ipc_planned_order` 单号 | `PeggingRecord.planned_order_id` |
| `ECS` | 最早开工日期 | `DATE` | Nullable | Earliest Constraint Start Date (最早瓶颈开工期)| `PeggingRecord.ecs_date` |
| `part_ready_date` | 最早物料齐套日 | `DATE` | Nullable | 所有子件到料并齐套的最早时刻 | `PeggingRecord.part_ready_date` |

#### 11. `ipc_alternate_allocation` (动态替代料分摊日志表)
> **业务说明**：详细记录 MRP 在扣减主料不足时，向一类/二类/三类替代料转移的具体物理分配量。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `main_part` | 原始请求主物料 | `VARCHAR(40)` | NOT NULL | BOM 原定主件编码 | `AlternateAllocationRecord.main_part_id` |
| `alt_part` | 实际被替代物料 | `VARCHAR(40)` | NOT NULL | 实际扣减的备选替代件编码 | `AlternateAllocationRecord.alt_part_id` |
| `allocated_qty` | 替代分配数量 | `DOUBLE` | Default `0.0` | 转移至替代件的物理扣减量 | `AlternateAllocationRecord.allocated_qty`|
| `day` | 分配发生天数 | `INTEGER` | NOT NULL | 排产发生的相对天数 | `AlternateAllocationRecord.day` |
| `alt_class` | 替代级别类型 | `INTEGER` | Default `1` | `1`-一类同质替代, `2`-二类降级, `3`-三类组替代| `AlternateAllocationRecord.alt_class` |

#### 12. `ipc_dispatch_ledger` (DBD 车间机台派工账本表)
> **业务说明**：DBD 车间派程引擎输出的物理派工单账本，记录工单在具体工作中心/机台上的绝对开完工时刻。

| 字段编码 (Field Code) | 字段物理名 | 物理类型 | 约束 / 主键 | 业务含义与计算逻辑 | 对应 C++ 内存结构 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `part_code` | 物料编码 | `VARCHAR(40)` | NOT NULL | 工单生产物料号 | `DispatchRecord.part_code` |
| `order_qty` | 排产工单数量 | `DOUBLE` | Default `0.0` | 车间实际派发数量 | `DispatchRecord.order_qty` |
| `original_start_day`| 理论开工天数 | `INTEGER` | NOT NULL | 理想无约束下的开工期 | `DispatchRecord.original_start_day` |
| `scheduled_day` | 实际锁定开工期 | `INTEGER` | NOT NULL | 考虑车间有限能力扣减后的锁定期 | `DispatchRecord.scheduled_day` |
| `allocated_capacity`| 占用机台工时 | `DOUBLE` | Default `0.0` | 消耗的物理机器小时 (Machine Hours) | `DispatchRecord.allocated_capacity` |
### 2.4 模块 2：联副产品分级优化表 (Co-product Optimization - 8 张表)
> **引擎感知与切片逻辑**：在半导体 Binning、化学提纯及炼化联副产品场景下，用于解构产出比例与降级消纳。

1. `ipc_coproduct_dimension`：联副产品特征维度明细表
2. `ipc_coproduct_grouping`：联副产品特征维度分组表
3. `ipc_coproduct_recipe`：联副产品配方与产出比例表 (`ratio_512`, `ratio_256`, `ratio_128`)
4. `ipc_coproduct_demand`：联副产品特定级别需求表
5. `ipc_coproduct_allocation`：联副产品实际分摊结果表
6. `ipc_coproduct_schedule`：联副产品批次排产计划表
7. `ipc_coproduct_config`：联副产品求解器参数表
8. `ipc_coproduct_yield`：收率与残渣损失评估表

---

### 2.5 模块 3：ETO 协同项目管理表 (ETO Project & WBS - 6 张表)
> **引擎感知与切片逻辑**：面向按项目设计 (Engineer-to-Order) 场景，绑定工程 WBS 节点与 CPM 关键路径。

1. `ipc_project`：主项目定义表
2. `ipc_project_group`：项目集群分类表
3. `ipc_project_manager`：项目经理与责任人表
4. `ipc_project_status`：项目里程碑状态跃迁表
5. `ipc_project_type`：项目交付类型表 (ETO / ATO / MTO)
6. `ipc_project_wbs`：WBS 元素任务表 (关联开完工期与工时占比)

---

### 2.6 模块 4：IBP 财务与预测共识表 (IBP Consolidated Planning - 30 张表)
> **引擎感知与切片逻辑**：感知外围 CRM/S&OP 数据，运行 Holt-Winters 与 3D 共识算法，形成财务 P&L 约束。

1. `ipc_consensus_forecast`：共识预测主表
2. `ipc_consensus_forecast_detail`：共识预测明细表 (按 Customer/Region/Part 展开)
3. `ipc_consensus_forecast_rolling_horizon`：滚动预测时间窗口表
4. `ipc_consensus_forecast_rolling_horizon_weight`：滚动预测历史权重表
5. `ipc_financial_ledger`：IBP 财业双生分类账簿表 (算 ROIC, NOPAT 与 营运资本)
6. `ipc_forecast`：基础预测流表
7. `ipc_forecast_causal_factor`：归因促进因子表 (促销/促销档期/宏观因子)
8. `ipc_forecast_consumption`：预测冲销规则配置表
9. `ipc_forecast_detail`：预测离散时间桶明细表
10. `ipc_sales_order_line`：客户销售订单行明细表
*(其余 20 张预测与财务明细表详见 [ipc_data_dictionary.md](file:///h:/IPC/docs/ipc_data_dictionary.md#4-ibp-财务与预测共识))*

---

### 2.7 模块 5：IO 多阶安全库存水位优化表 (IO Safety Stock Policy - 12 张表)
> **引擎感知与切片逻辑**：运行 MEIO 算法，将服务水准 $SL$ 转化为天级安全库存目标水位。

1. `ipc_io_dos_policy`：DOS (Day-of-Supply) 库存天数策略表
2. `ipc_io_safety_stock_average_demand_profile`：平均需求波动轮廓表
3. `ipc_io_safety_stock_item`：物料站点级安全库存求解目标表 ($SS_i$)
4. `ipc_io_safety_stock_item_mapping`：安全库存上游依赖映射表
5. `ipc_io_safety_stock_time_phased_bounds`：时相化安全库存上下界护栏表
6. `ipc_io_safety_stock_time_phased_result`：天级时相化安全库存水位输出表
7. `ipc_io_ss_rule`：安全库存计算公式规则库 (`FIXED`, `PERCENT_DEMAND`, `MEIO_DOD`)
*(其余 5 张 IO 辅助配置表详见 [ipc_data_dictionary.md](file:///h:/IPC/docs/ipc_data_dictionary.md#5-io-安全库存水位优化))*

---

### 2.8 模块 6：基础支撑、感知整合与主数据表 (Master Data & Perception - 133 张表)
> **引擎感知与切片逻辑**：负责感知外围 ERP/MES/WMS/CRM 系统全样数据，实施数据清洗与解耦映射。

* **站点与地理**：`ipc_site`, `ipc_location`, `ipc_logistic_location`, `ipc_region`, `ipc_country`
* **客户与供应商**：`ipc_customer`, `ipc_hierarchy_customer`, `ipc_supplier`, `ipc_supplier_part`
* **资源与能力**：`ipc_work_center`, `ipc_work_center_capacity`, `ipc_constraint`, `ipc_constraint_available`, `ipc_constraint_assignment`
* **日历与时间**：`ipc_sop_planning_calendar`, `ipc_sop_calendar_date`, `ipc_shift_calendar`
* **工程变更 (ECN)**：`ipc_ecn_effectivity`, `ipc_ecn_substitute_map`, `ipc_ecn_phaseout_ledger`
* **呆滞置换 (SWAP)**：`ipc_swap_result`, `ipc_swap_candidate_pool`, `ipc_swap_rule_config`
* **沙盒与审计**：`ipc_collab_scenario`, `ipc_collab_audit_log`, `ipc_solver_config`
*(完整 133 张主数据与感知物理表字典详见 [ipc_data_dictionary.md](file:///h:/IPC/docs/ipc_data_dictionary.md#6-基础支撑与主数据))*

---

## 三、 运营计划 (Operational Plan / IBP & ITP) 数据结构与算法详解

运营计划 (Operational Plan) 贯穿从战略 IBP 财业共识预测，到 ITP 战术配额防波堤拦截的全过程，核心在于**消除预测与订单拉动的牛鞭效应，并建立刚性财务 ROIC 护栏**。

### 3.1 业务算法推导与流程解构

#### 1. 共识预测与 Holt-Winters 时序平滑算法
运营计划点火时，首先融合 Sales, Marketing, Statistical 与 Strategic 四维预测向量。针对统计时序，运行 **Holt-Winters 三重指数平滑算子** 提取水平 ($L_t$)、趋势 ($T_t$) 与季节性因子 ($S_t$)：
$$L_t = \alpha \frac{Y_t}{S_{t-p}} + (1-\alpha)(L_{t-1} + T_{t-1})$$
$$T_t = \beta (L_t - L_{t-1}) + (1-\beta) T_{t-1}$$
$$S_t = \gamma \frac{Y_t}{L_t} + (1-\gamma) S_{t-p}$$
预测值计算出来后，调用历史比例拆解算子，将其拆解至具体叶子 SKU 节点。

#### 2. MEIO 多阶安全库存方差前向传递算法
为了防止各节点独立设置安全库存导致全网资金挤压，MEIO 沿着 BOM 拓扑 DAG 自顶向下传递需求均值与方差：
$$SS_i = Z_i \cdot \sqrt{L_i \sigma_{D,i}^2 + D_i^2 \sigma_{L,i}^2}$$
其中 $Z_i$ 由服务水准 $SL_i$ 经标准正态分布反函数算得。多阶方差在连续一维数组中累加，实现全局资金占用最小化。

#### 3. 3D 需求张量与 Forecast Consumption 滑动窗口冲销
为了防止预测需求与后到的客户订单重复拉动供应链，引擎构建 3D DOD 需求张量 `gross_demand[part_id][day][dim]`，运行双向滑动冲销窗口（Sliding Window）：
$$\text{Unconsumed Forecast}(t) = \max\left( 0, \text{Original Forecast}(t) - \sum_{k=-\Delta_1}^{+\Delta_2} \text{Customer Orders}(t + k) \right)$$
冲销完成后，输出纯净的 Net Demand Stream。

#### 4. ITP 战术配额 5 大条件门控树 (Allotment Decision Tree)
当净需求流进入战术配额计算时，依次通过 5 大刚性判决门控：
1. **Gating 瓶颈判定**：校验需求总量是否超出网络瓶颈产能。若无瓶颈，直接按 FIFS (先到先得) 顺畅释放。
2. **时间桶属性判定**：
   - **短期 (1-4 周)**：触发 Strategic Priority 算子，High 优先级客户承诺 100% 划拨配额，剩余产能放入 Surplus 池。
   - **长期 (5 周+)**：触发 FairShare 比例分摊算子：$\text{Assigned}_i = \text{Supply} \cdot \frac{\text{Demand}_i}{\sum \text{Demand}}$。
3. **跨时间桶 Netting 判定**：若存在“前缺后富” ($\text{Shortage}(t) > 0$ 且 $\text{Surplus}(t+k) > 0$)，触发 **Left-Shifting 向左拉动算子**，提前切片占用多余产能。
4. **配额落库**：生成刚性战术配额护栏 `ipc_allotment_constraint`。

---

### 3.2 运营计划专用 C++ DOD 物理数据结构

```cpp
// 1. 运营计划：对应 PartSite 的 SoA 物理对齐结构体 (64-byte Cache-line 对齐)
struct alignas(64) PartSiteRecord {
    uint32_t part_id;               // 物料 ID
    std::string part_code;          // 物料物理编码
    double on_hand;                 // 在库现有量
    double lead_time;               // 制造提前期
    std::string mrp_rule;           // MRP 消纳规则
    std::string part_type;          // FINISHED, SEMI, RAW, ALT
    bool is_phantom = false;        // 是否虚拟件
    double cost = 1.0;              // 销售单价/成本
    std::string site = "SITE_001";  // 归属厂区
    double transshipment_cost = 0.0; // 厂区调拨成本
    int transshipment_lead_time = 0; // 调拨提前期
    double safety_stock = 0.0;      // MEIO 安全库存水位
};

// 2. 运营计划：配额哈希 Fast-Path 查找键与专用位移 Hash 结构
struct AllotmentConstraintKey {
    uint32_t part_id;
    uint32_t site_id;
    uint32_t region_id;
    uint32_t cust_group_id;
    uint32_t family_id;
    int day;
    std::string scenario = "BASE";

    bool operator==(const AllotmentConstraintKey& o) const {
        return part_id == o.part_id && site_id == o.site_id &&
               region_id == o.region_id && cust_group_id == o.cust_group_id &&
               family_id == o.family_id && day == o.day && scenario == o.scenario;
    }
};

struct AllotmentConstraintKeyHash {
    size_t operator()(const AllotmentConstraintKey& k) const {
        size_t h1 = std::hash<uint32_t>{}(k.part_id);
        size_t h2 = std::hash<int>{}(k.day);
        size_t h3 = std::hash<uint32_t>{}(k.cust_group_id);
        size_t h4 = std::hash<uint32_t>{}(k.region_id);
        size_t h5 = std::hash<uint32_t>{}(k.family_id);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
    }
};

// 3. 运营计划：RAII 零堆分配事务回滚 Guard (配额扣减失败自动撤销)
class AllotmentRollbackGuard {
private:
    std::unordered_map<AllotmentConstraintKey, double, AllotmentConstraintKeyHash>& target_map;
    std::vector<std::pair<AllotmentConstraintKey, double>> rollback_stack;
    bool committed = false;

public:
    AllotmentRollbackGuard(std::unordered_map<AllotmentConstraintKey, double, AllotmentConstraintKeyHash>& map)
        : target_map(map) {}

    void record_deduction(const AllotmentConstraintKey& key, double amount) {
        rollback_stack.push_back({key, amount});
    }

    void commit() { committed = true; }

    ~AllotmentRollbackGuard() {
        if (!committed) {
            for (auto it = rollback_stack.rbegin(); it != rollback_stack.rend(); ++it) {
                target_map[it->first] += it->second; // 还原已被扣减的配额
            }
        }
    }
};
```

---

## 四、 执行计划 (Execution Plan / IOP & DBD) 数据结构与算法详解

执行计划 (Execution Plan) 聚焦于 IOP 微观消纳、有限能力 MRP 展开、多级替换料分摊、交期定锚与 DBD 车间派程，核心在于**极速吞吐与毫秒级防死锁求解**。

### 4.1 业务算法推导与流程解构

#### 1. 有限能力 LBL MRP 拓扑展开与 OpenMP CAS 无锁消纳
消纳引擎按低层码 (LLC) 逐层计算。在同一层级内，物料节点相互独立，系统触发 OpenMP 多线程并行消纳 (`#pragma omp parallel for`)。为了防止多线程并发扣减库存与在途时的锁竞争，引入 CAS (Compare-And-Swap) 无锁原子操作：
```cpp
bool allocate_stock_cas(std::atomic<uint64_t>& raw_bits, double request_qty) {
    uint64_t current_bits = raw_bits.load(std::memory_order_relaxed);
    double current_val, target_val;
    uint64_t target_bits;
    do {
        std::memcpy(&current_val, &current_bits, sizeof(double));
        if (current_val < request_qty) return false; // 水位不足，消纳失败
        target_val = current_val - request_qty;
        std::memcpy(&target_bits, &target_val, sizeof(double));
    } while (!raw_bits.compare_exchange_weak(current_bits, target_bits,
             std::memory_order_release, std::memory_order_acquire));
    return true;
}
```

#### 2. 多级替代料 Lot-size 重归一化分摊算法
当主料发生缺口时，引擎扫描 BOM 中的 `alt_grp`。若存在多个可替代物料，首先计算各替代件的目标分摊比例，并根据包装规格 ($lot\_size$) 与损耗率进行向上取整：
$$\text{Actual\_Qty}_i = \left\lceil \frac{\text{Due\_Qty} \cdot perqty_i \cdot (1 + scrap_i)}{lot\_size_i} \right\rceil \cdot lot\_size_i$$
因取整多出的物理残差反向抵消上层需求。已决策部件移出活跃集后，对其余替代件实施**重归一化 (Re-normalization)**：
$$Ratio_i^{new} = \frac{target_i}{\sum_{k \in \text{Remaining}} target_k}$$

#### 3. 交期契约定锚与 4 步资源置换 (Resource Swapping)
- **门控 4 (PSD 交期契约定锚)**：比较真实 Due Date 与 CTP 探路返回的 Best Can Do，定锚不可撕裂的交期红线：
  $$\text{PSD} = \max(\text{Due Date}, \text{Best Can Do})$$
- **门控 5 (微观扰动与 Resource Swapping 4 步置换)**：当高优先级订单插单或供应链发生断供扰动时，触发置换闭环：
  1. **高优先插单判定**：校验新订单的 `encode_composite_priority` 复合位打包优先级。
  2. **全网软预留扫描**：扫描已分配资源中优先级低于当前订单的软预留 (Soft Reservation)。
  3. **低优先资源剥离 (De-allocation)**：强行剥离低优先级订单占用的在库/在途/产能配额，释放给高优先订单。
  4. **二次排期**：被剥离的低优先订单重新进入求精队列，在 Allotment Cap 允许范围内寻找新的 ATP 日期。

#### 4. DBD 车间派程与 180 秒 DuckDB 流式写回
DBD 引擎将计划订单展开为具体的工序 Operation。根据工作中心/机台的日产能 (`wc_daily_capacity`) 构建工单 DAG 依赖图，求解开完工时刻。最后调用 `duckdb::Appender` 原生二进制流 API，在 180 秒内完成百万人份派工单向物理 DB 的流式写回。

---

### 4.2 执行计划专用 C++ DOD 物理数据结构

```cpp
// 1. 执行计划：扁平化 BOM 节点 (C++ DOD Layout, 255-byte 物理连续对齐)
struct FlatBomItem {
    uint32_t parent_id;          // 父物料 Hash ID
    uint32_t child_id;           // 子物料 Hash ID
    double per_qty;              // 单件消耗量 perqty
    double scrap;                // 损耗率 scrap
    int alt_group_id = -1;       // 替代组逻辑 ID
    int alt_priority = 0;        // 替代优先级
    double target_ratio = 1.0;   // 目标分配比例
    double historical_qty = 0.0; // 累计已消耗量
    double lot_size = 0.0;       // 包装规格批值
    int eff_start_day = -1;      // 生效起始天数
    int eff_end_day = -1;        // 生效失效天数
    double ltb_limit = -1.0;     // LTB 买入上限
};

// 2. 执行计划：建议计划订单 (Planned Order C++ 紧凑内存结构)
struct PlannedOrder {
    uint32_t order_id;            // 工单 ID
    uint32_t part_id;             // 物料 ID
    uint32_t site_id;             // 站点 ID
    double qty;                   // 计划生成数量
    double eff_qty;               // 考虑损耗后的有效数量
    int start_day;                // 建议开工天数
    int finish_day;               // 建议完工天数
    double dimension_val;         // 特征维度值
    std::string source = "MAKE";  // MAKE, BUY, TRANSFER
};

// 3. 执行计划：钉结分配明细 (Pegging Record 内存栈结构)
struct PeggingRecord {
    uint32_t demand_id;           // 独立需求 ID
    uint32_t part_id;             // 实际分配物料 ID
    double qty;                   // 绑定分配数量
    int day;                      // 承诺交付天数
    std::string planned_order_id; // 关联的计划订单单号
};

// 4. 执行计划：64 位复合优先级位打包函数 (CPU 100% 硬件标量比较)
inline uint64_t encode_composite_priority(bool is_committed, int customer_tier, int due_day, int original_priority, double revenue) {
    uint64_t committed_bit = is_committed ? 0ULL : 1ULL; // Bit 62 (契约状态)
    uint64_t tier_val = (static_cast<uint64_t>(customer_tier) & 0x3ULL); // Bit 60-61 (客户等级)
    uint64_t due_val = (static_cast<uint64_t>(due_day) & 0xFFFFULL); // Bit 44-59 (交付天数)
    uint64_t pri_val = (static_cast<uint64_t>(original_priority) & 0xFFFFULL); // Bit 28-43 (原始优先级)
    uint64_t rev_val = (static_cast<uint64_t>(revenue) & 0xFFFFFFFULL); // Bit 0-27 (金额)
    return (committed_bit << 62) | (tier_val << 60) | (due_val << 44) | (pri_val << 28) | rev_val;
}
```

---

## 五、 C++ 代码与算法底层调优全景账本 (IPC Architecture & Engineering Ledger)

| 阶段序号 | 计划分类 | 业务算法 / 数据模型 | 底层 C++ 数据结构与优化技术 | 关联 DuckDB 物理表 | 源码文件与行号 | 对账状态 |
| :---: | :---: | :--- | :--- | :--- | :--- | :---: |
| **0.0** | 前置感知 | 外围 ERP/MES/WMS/CRM 感知清洗与数据整合 | DuckDB `199` 表全网 ETL 归一化映射 | 全网 199 张 DuckDB 物理表 | [database.cpp:L1](file:///h:/IPC/src/database.cpp#L1) | 【已实装】 |
| **0.1** | 前置准备 | 订单优先级与战略维度绑定 | `encode_composite_priority` (64位复合位打包) | `ipc_independent_demand` | [ipc_types.h:L220](file:///h:/IPC/include/ipc_types.h#L220) | 【已实装】 |
| **0.2** | 前置准备 | 全网物料 LLC 低层码拓扑编译 | `compile_low_level_codes` (DAG 深度优先排序) | `ipc_part` / `ipc_bom_item` | [lsc_tree.cpp:L105](file:///h:/IPC/src/lsc_tree.cpp#L105) | 【已实装】 |
| **1.1** | 运营计划 | 多源共识预测历史比例拆解 | `ConsensusForecast` & `HierarchyResolver` | `ipc_hierarchy_product_family` | [engine_main.cpp:L876](file:///h:/IPC/src/engine_main.cpp#L876) | 【已实装】 |
| **1.2** | 运营计划 | Holt-Winters 时序平滑预测 | `run_holt_winters` (三重指数平滑算子) | `ipc_sop_calendar_date` | [math_utils.cpp:L45](file:///h:/IPC/src/math_utils.cpp#L45) | 【已实装】 |
| **1.3** | 运营计划 | MEIO 多阶安全库存方差传递 | `parts_daily_demand` 一维展平连续数组 $O(1)$ | `ipc_part` / `ipc_part_site` | [engine_main.cpp:L1195](file:///h:/IPC/src/engine_main.cpp#L1195) | 【已实装】 |
| **2.1** | 运营计划 | 3D DOD 需求张量构建 | `gross_demand[part_id][day][dim]` 3D 向量 | `ipc_independent_demand` | [mrp_engine.cpp:L54](file:///h:/IPC/src/mrp_engine.cpp#L54) | 【已实装】 |
| **2.2** | 运营计划 | Forecast Consumption 滑动冲销 | 双向指针滑动窗口消纳算子 | `ipc_independent_demand` | [mrp_engine.cpp:L70](file:///h:/IPC/src/mrp_engine.cpp#L70) | 【已实装】 |
| **3.1** | 运营计划 | ITP 战术 5 大门控与防波堤生成 | `AllotmentConstraintKeyHash` 星期一 Fast-Path | `ipc_allotment_constraint` | [ipc_types.h:L115](file:///h:/IPC/include/ipc_types.h#L115) | 【已实装】 |
| **3.2** | 运营计划 | 战术配额事务性尝试与撤销 | `AllotmentRollbackGuard` RAII 零分配 Guard | `ipc_allotment_ledger` | [ipc_types.h:L196](file:///h:/IPC/include/ipc_types.h#L196) | 【已实装】 |
| **4.1** | 执行计划 | LBL MRP 逐层消纳与库存扣减 | OpenMP `#pragma omp parallel for` + CAS 无锁 | `ipc_onhand` / `ipc_scheduled_receipt` | [mrp_engine.cpp:L161](file:///h:/IPC/src/mrp_engine.cpp#L161) | 【已实装】 |
| **4.2** | 执行计划 | 多级动态替换料 BOM 分摊 | `AlternateAllocationRecord` 与 Alt 组别检索 | `ipc_bom_item` / `ipc_alternate_allocation` | [substitution.cpp:L30](file:///h:/IPC/src/substitution.cpp#L30) | 【已实装】 |
| **4.3** | 执行计划 | 建议计划订单生成与拆单 | `PlannedOrder` & `PlannedOrderSplit` 零堆分配 | `ipc_planned_order` / `ipc_planned_supply_assignment` | [mrp_engine.cpp:L340](file:///h:/IPC/src/mrp_engine.cpp#L340) | 【已实装】 |
| **5.1** | 执行计划 | IOP 交期契约红线定锚 PSD | `PSD = max(Due Date, Best Can Do)` 定锚算法 | `ipc_planned_supply_assignment` | [mrp_engine.cpp:L583](file:///h:/IPC/src/mrp_engine.cpp#L583) | 【已实装】 |
| **5.2** | 执行计划 | Resource Swapping 4-Step 动态置换 | 软预留 `gross_demand_priority` 动态剥离 | `ipc_swap_result` | [mrp_engine.cpp:L610](file:///h:/IPC/src/mrp_engine.cpp#L610) | 【已实装】 |
| **6.1** | 执行计划 | DBD 车间工位/机台派程 | `run_dbd_dispatch_engine` & Workcenter 匹配 | `ipc_dispatch_ledger` / `ipc_work_center` | [dbd_engine.cpp:L1](file:///h:/IPC/src/dbd_engine.cpp#L1) | 【已实装】 |
| **6.2** | 执行计划 | 闭环 MPC 180 秒工单刚性反写 | DuckDB `duckdb::Appender` 原生二进制流式写回 | `ipc_dispatch_ledger` / `ipc_planned_order` | [engine_main.cpp:L1335](file:///h:/IPC/src/engine_main.cpp#L1335) | 【已实装】 |

---
*(本文档包含 IPC 持久化数据库全量表结构、运营计划与执行计划数据结构与算法细节，为 IPC 专栏标准对账文档)*
