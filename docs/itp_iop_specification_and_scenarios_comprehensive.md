# IPC 智能计划与控制（ITP/IOP）双阶段计划物理引擎技术规范与核心场景白皮书

本技术白皮书是 Intelligent Planning & Control (IPC) 系统统御求解器内核的官方开发与业务逻辑规格说明书。为了方便供应链管理（SCM）专家与计划专家审计求解器的逻辑自洽性，本手册**完全舍弃 C++ 裸金属层的数据结构与底层性能优化细节（如 列式数据模型 对齐、内存屏障、多线程无锁、并行计算 指令等）**，聚焦于表达 SCM 的 **逻辑数据模型**、**业务计划算法**，以及其在 13 个物理执行流阶段下的 **核心业务场景** 与 **计算对账产出**。

---

## 🗺️ 一、 ITP/IOP 同构设计与供应链网络建模的深度技术规范

在离散制造供应链体系中，计划与执行的脱节往往是因为两套系统（战术层 MPS/MRP 与执行层 Scheduling）采用了截然不同的物理网状模型。IPC 系统从底层数学形式上统一了战术（ITP）与滚动执行（IOP）的供应链图网络建模。

### 1. ITP 与 IOP 的定位与差异
ITP 与 IOP 采用完全一致的供应链网络拓扑及约束校验逻辑，但解决不同时间跨度与业务边界的问题：
* **ITP (Intelligent Tactical Planning, 智能战术计划)**：
  - **时间范围与频度**：解决中远期（如 30-180 天）大盘滚动规划与供需平衡。
  - **核心职责**：在粗产能和物料大池约束下进行物料爆炸净算（LBL），通过 ITP 瓶颈检测与 VVIP/Fair-Share 产能比例缩放，输出大盘战术配额，并写入 Allotment 配额确权账本。
* **IOP (Intelligent Operational Planning, 智能滚动排产计划)**：
  - **时间范围与频度**：解决短期（如 1-14 天）车间级精密滚动排程与微观消纳。
  - **核心职责**：在 rigid（刚性）的 Allotment 配额确权保护下，对 Planned Orders 进行微观时序派程（时序 ATP 预占与 CTP 交期应答），执行 Setup Waiver（换模工时豁免）与 OTP（低优订单平移抢占），确定每笔订单的精准上线时间与物理资源占用。

### 2. 供应链网络基础拓扑建模
全网节点（物料 SKU、站点、物理工作中心/产能资源）和连接边（BOM/替代关系/调拨路线）在内存中通过面向数据的扁平倒排索引进行映射，构成逻辑拓扑树（LSC Tree）。
* **物料与工艺路线边**：成品零件 $P$ 展开至子件 $C$，包含单位用量 $PerQty_{P \rightarrow C}$，损耗率 $Scrap_{P \rightarrow C}$。
* **时空替代边**：若子件 $C$ 属于替代组 $AltGrp$，则替代组中存在多个候选件，各自具有替代优先级 $AltPriority$、目标配额比例 $TargetRatio$、累计历史分配量 $H$。

### 3. 全局低层码 (LLC) Bellman-Ford 算法推演
低层码（Low Level Code, LLC）是指物料在所有 BOM 树中所处的最低层级层码。成品层码为 0，组件层码大于等于其所有父件的最大层码加 1。LLC 编译的正确性直接决定了 MRP 级联计算时数据是否会发生遗漏。

在 IPC 同构网络中，系统通过 **Bellman-Ford 拓扑松弛算法** 动态计算全网物料的 LLC。
* **状态定义**：令全网物料顶点集合为 $V$，BOM 拓扑依赖边集合为 $E$（其中边 $e: P \rightarrow C$ 表示父件 $P$ 消耗子件 $C$）。每个物料 $v \in V$ 的低层码记为 $LLC(v)$，初始值对于所有物料均设为 $0$。
* **松弛方程**：对于所有的 BOM 关系边 $(P \rightarrow C) \in E$，迭代执行以下松弛计算：
  $$ LLC(C) \leftarrow \max \Big( LLC(C), LLC(P) + 1 \Big) $$
* **死锁与环路熔断机制**：为防止排产数据录入错误引起循环 BOM 死锁（例如物料 A 消耗物料 B，物料 B 又消耗物料 A），松弛计算的迭代轮数上限设为 $100$。若在第 $100$ 轮松弛时仍有节点的 LLC 值发生变化，求解器会强制终止运行并抛出如下致命异常：
  `[致命死锁] BOM 拓扑编译中检测到闭环死锁依赖环路！系统强行熔断。`
  计划员必须通过 DBeaver 运行拓扑环路检测 SQL 审计数据。

---

## 💾 二、 供应链物理数据模型字段映射与 SCM 业务规则深度指南

以下详尽陈述 DuckDB 内存账本中 17 个核心数据表的所有字段属性、默认值约束与 SCM 计划业务规则，计划员可通过 overrides 重写这些输入字段来直接干预求解器决策。

### 1. 物料站点主数据表 (`ipc_material_node` / 对应数据库物理表 `ipc_material_node`)
物料在特定厂区站点下的计划控制属性与安全库存规则。

| 字段物理名 (FieldName) | 物理类型 (DataType) | 必填/键 (Key) | 详细业务备注与 SCM 规则 (Comment) |
| :--- | :--- | :--- | :--- |
| `part` | VARCHAR | 🔑 PK | 物料唯一编码 (SKU)。 |
| `site` | VARCHAR | 🔑 PK | 厂区站点代码 (如 SITE_001)。 |
| `part_type` | VARCHAR | Nullable | 物料类型分类：`FINISHED` 成品, `SEMI` 半成品, `RAW` 原料, `ALT` 替代件。 |
| `mrp_rule` | VARCHAR | Nullable | 净需求计算规则。默认值 `MRP`。 |
| `is_phantom` | BOOLEAN | Nullable | 虚拟件标识。若为 `true`，直接穿透分解。 |
| `selling_ave_price` | DOUBLE | Nullable | 平均销售单价，用于持有资本价值核算。 |
| `transshipment_cost` | DOUBLE | Nullable | 跨厂区调拨每单位物料的运费成本。 |
| `transshipment_lead_time` | INTEGER | Nullable | 跨厂区调拨所需的固定在途天数。 |
| `run_rate` | DOUBLE | Nullable | 变动工时系数（小时/件）。用于提前期批量拉伸。 |
| `lead_time` | DOUBLE | Nullable | 固定生产或采购提前期天数。 |
| `round_to_integer` | BOOLEAN | Nullable | 计划订单数量是否强制向上取整。 |
| `on_hand_type` | VARCHAR | Nullable | 在手机制类别。默认 `Standard`。若设为 `Exclude`，消纳时强制忽略现有量。 |
| `time_fence_days` | INTEGER | Nullable | 计划冻结时界。求解器生成的 Planned Order 开工时间严格大于 RunDate + Fence。 |
| `sourcing_policy` | VARCHAR | Nullable | 寻源策略。默认 `Standard`。 |
| `safety_stock` | DOUBLE | Nullable | 运行期计算出的安全库存目标水位。 |
| `ss_fixed_qty` | DOUBLE | Nullable | 计划员手工指定的固定安全库存。 |
| `ss_rule` | VARCHAR | Nullable | 安全库存计算规则。`None`、`Fixed`（固定值）、`Dynamic`（MEIO多级计算）。 |
| `dos_policy` | VARCHAR | Nullable | 天数供应法规则。默认 `None`。若设为 `DAYS_OF_SUPPLY`，由 DOS 天数覆盖动态计算。 |
| `dos_intervals` | DOUBLE | Nullable | DOS 天数覆盖参数。默认 `0.0`。 |
| `planning_calendar` | VARCHAR | Nullable | 绑定的工厂日历代码。默认 `DEFAULT`。 |
| `safety_stock_value` | DOUBLE | Nullable | 安全库存资本占用额度（计算列：`safety_stock * selling_ave_price`）。 |

### 2. 在手现有库存表 (`ipc_onhand`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `location` | VARCHAR | 🔑 PK | 库房库位名称。 |
| `part` | VARCHAR | 🔑 PK | 对应的物料零件号。 |
| `site` | VARCHAR | 🔑 PK | 所属站点。 |
| `available_date` | DATE | PK | 库存可用日期。 |
| `qty` | DOUBLE | Nullable | 现有量库存数量（必须 $\ge 0.0$）。 |
| `inventory_type` | VARCHAR | Nullable | 库存质量分类。默认 `Standard`。 |

### 3. 在途订单供应表 (`ipc_scheduled_receipt`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `sr_id` | VARCHAR | 🔑 PK | 唯一在途单号。 |
| `to_part` | VARCHAR | Nullable | 接收的物料。 |
| `qty` | DOUBLE | Nullable | 在途供应量。 |
| `to_site` | VARCHAR | Nullable | 接收站点。 |
| `request_due_date` | DATE | Nullable | 合同约定交付就绪日期。 |
| `supply_status` | VARCHAR | Nullable | 在途状态（如 `In-Transit`, `Shipped`, `Confirmed`）。若为 `Ignore` 则求解器忽略此在途。 |

### 4. BOM 子项明细表 (`ipc_bom_item`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `bomid` | VARCHAR | 🔑 PK | BOM 配方编码。 |
| `site` | VARCHAR | 🔑 PK | 站点代码。 |
| `component` | VARCHAR | 🔑 PK | 子组件物料编码。 |
| `perqty` | DOUBLE | Nullable | 单件装配消耗量。默认 `1.0`。 |
| `scrap` | DOUBLE | Nullable | 装配损耗率（如 0.05 代表 5% 损耗）。 |
| `alt_grp` | VARCHAR | Nullable | 替代组编码。属于同组的组件互为可替换物料。 |
| `priority` | INTEGER | Nullable | 替代类型。`1` Class 1 偏离度最小；`2` Class 2 供应商评级优先；`3` Class 3 批量规整归一化。 |
| `target` | DOUBLE | Nullable | 替代分摊比率目标（如 0.60 代表期望分摊 60% 份额）。 |
| `alt_todate_qty` | DOUBLE | Nullable | 历史累计已替代分摊数量。默认 `0.0`。 |
| `lot_size` | DOUBLE | Nullable | 替代发生时的起订批量限制。默认 `0.0`。 |
| `eff_start_day` | INTEGER | Nullable | ECN 生效开始的天偏移量。 |
| `eff_end_day` | INTEGER | Nullable | ECN 切割失效的天偏移量。 |
| `ltb_limit` | DOUBLE | Nullable | Last-Time-Buy 呆滞料限额限制。 |
| `mix_group_id` | INTEGER | Nullable | 混合替代物料优先级组。 |
| `relationship_type` | VARCHAR | Nullable | 替代刚性。`alt` 硬替代（受ECN限制）；`soft` 软替代（允许Swap跨站点调拨）。 |

### 5. BOM 工艺路线表 (`ipc_bom_route`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `site` | VARCHAR | 🔑 PK | 站点代码。 |
| `part` | VARCHAR | 🔑 PK | 成品装配 SKU 编码。 |
| `bomid` | VARCHAR | PK | 绑定的 BOM 编码。 |
| `priority` | INTEGER | Nullable | 工艺路线选择优先级。默认 `1`。 |
| `bom_type` | VARCHAR | Nullable | 配方类别。 |

### 6. 独立需求表 (`ipc_independent_demand`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `demand` | VARCHAR | 🔑 PK | 需求订单号。 |
| `item` | DOUBLE | 🔑 PK | 需求行项项目值。默认 `1.0`。 |
| `part` | VARCHAR | Nullable | 需求物料编码。 |
| `par_site` | VARCHAR | Nullable | 需求所属母站点代码。 |
| `customer` | VARCHAR | Nullable | 客户编码。 |
| `request_delivery_date` | DATE | Nullable | 期望的交货日期。 |
| `request_due_date` | DATE | Nullable | 期望的可承诺交付日期。 |
| `open_qty` | DOUBLE | Nullable | 尚未交付交付的敞口缺口数量。 |
| `request_qty` | DOUBLE | Nullable | 原始需求订单数量。 |
| `status` | VARCHAR | Nullable | 订单状态（如 `Open`, `Committed`）。 |
| `order_priority` | INTEGER | Nullable | 原始 ERP 优先级数值（如 100 默认）。 |
| `site` | VARCHAR | Nullable | 厂区编码。 |
| `dimension_grp` | VARCHAR | Nullable | IOP 时序匹配中产品规格维度组。 |
| `preference_mode` | VARCHAR | Nullable | IOP 时序匹配偏好模式。默认 `N`（就近）、`Z`（优先级）、`C`（可得性）。 |
| `customer_tier` | INTEGER | Nullable | 客户层级等级。`0` VVIP 战略保护、`1` VIP、`2` 普通、`3` 长尾。 |
| `revenue` | DOUBLE | Nullable | 预估订单销售金额。默认 `0.0`。 |

### 7. 战术确权配额控制表 (`ipc_allotment_constraint`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `scenario_id` | VARCHAR | 🔑 PK | 场景沙箱 ID。 |
| `part_code` | VARCHAR | 🔑 PK | 受控配额的物料编码。 |
| `site_code` | VARCHAR | 🔑 PK | 站点。 |
| `region` | VARCHAR | 🔑 PK | 区域。默认 `*`。 |
| `customer_group` | VARCHAR | 🔑 PK | 战略客户组群。默认 `*`。 |
| `product_family` | VARCHAR | 🔑 PK | 产品家族。 |
| `day` | INTEGER | 🔑 PK | 天偏移。 |
| `itp_calculated_qty` | DOUBLE | Nullable | 由 ITP 战术计划运行算出的理论配额上限。 |
| `override_qty` | DOUBLE | Nullable | 计划员手工锁定 overrides 配额参数。 |
| `is_locked` | BOOLEAN | Nullable | 是否刚性锁定。若为 true，求解器拒绝在此基础上映增。 |

### 8. 战术确权配额执行账本表 (`ipc_allotment_ledger`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `scenario_id` | VARCHAR | 🔑 PK | 场景沙箱 ID。 |
| `part_code` | VARCHAR | 🔑 PK | 受控配额物料。 |
| `site_code` | VARCHAR | 🔑 PK | 站点。 |
| `region` | VARCHAR | 🔑 PK | 区域。默认 `*`。 |
| `customer_group` | VARCHAR | 🔑 PK | 战略客户群。默认 `*`。 |
| `product_family` | VARCHAR | 🔑 PK | 产品族。 |
| `day` | INTEGER | 🔑 PK | 时段（天偏移）。 |
| `allotment_limit` | DOUBLE | Nullable | 生效的限额值。若 override 生效则取 override；否则取 itp 计算值。 |
| `consumed_qty` | DOUBLE | Nullable | IOP 滚动排产中已经被订单实际扣减占用的配额量。 |
| `available_qty` | DOUBLE | Nullable | 剩余可用配额水位（计算列：`allotment_limit - consumed_qty`）。 |
| `blocked_demand_qty` | DOUBLE | Nullable | 由于配额耗尽而遭到阻断/无法承诺的订单数量。 |

### 9. 建议计划订单表 (`ipc_planned_order`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `ipc_planned_order` | VARCHAR | 🔑 PK | 唯一计划订单号。 |
| `request_start_date` | DATE | Nullable | 计划的最早建议开工日期（已扣除提前期）。 |
| `due_date` | DATE | Nullable | 计划的期望就绪入库完工日期。 |
| `qty` | DOUBLE | Nullable | 计划建议的补充数量。 |
| `eff_qty` | DOUBLE | Nullable | 考虑装配损耗率之后的实际投入量。 |
| `dimension_grp` | VARCHAR | Nullable | 晶圆多规格降级匹配中携带的规格维度组。 |
| `is_planned` | BOOLEAN | Nullable | 是否为有效订单。 |
| `part` | VARCHAR | Nullable | 物料补货 SKU 编码。 |
| `source` | VARCHAR | Nullable | 供应来源渠道。 |
| `site` | VARCHAR | Nullable | 站点编码。 |

### 10. 计划供应钉结分配表 (`ipc_planned_supply_assignment`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `demand` | VARCHAR | 🔑 PK | 被匹配满足的需求编号。 |
| `item` | DOUBLE | 🔑 PK | 需求行项目号。 |
| `part` | VARCHAR | 🔑 PK | 供应物料唯一编码。 |
| `site` | VARCHAR | 🔑 PK | 站点编码。 |
| `location` | VARCHAR | Nullable | 库位编码。 |
| `due_date` | DATE | Nullable | 期望就绪日期。 |
| `supply` | VARCHAR | Nullable | 供给单号。 |
| `supply_type` | VARCHAR | Nullable | 供应类型（此处为 `PlannedOrder`）。 |
| `assigned_qty` | DOUBLE | PK | 本次钉结分配的实际数量。 |
| `dimension_grp` | VARCHAR | Nullable | 消纳维度组。 |
| `available_date` | DATE | Nullable | 实际供应可用日期 (ATP Date)。 |
| `ipc_planned_order` | VARCHAR | Nullable | 关联的计划订单号。 |
| `part_ready_date` | DATE | Nullable | 最早物料齐套日期。 |

### 11. 锁定供需钉结分配表 (`ipc_supply_assignment`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `demand` | VARCHAR | 🔑 PK | 需求编号。 |
| `item` | DOUBLE | 🔑 PK | 需求行项目号。 |
| `part` | VARCHAR | 🔑 PK | 供应物料唯一编码。 |
| `site` | VARCHAR | 🔑 PK | 站点编码。 |
| `ind_part` | VARCHAR | Nullable | 独立需求物料。 |
| `location` | VARCHAR | Nullable | 库位编码。 |
| `due_date` | DATE | Nullable | 期望就绪日期。 |
| `supply` | VARCHAR | Nullable | 供给单号。 |
| `supply_type` | VARCHAR | Nullable | 供应类型（如 `OnHand` 或 `ScheduledReceipt`）。 |
| `assigned_qty` | DOUBLE | PK | 本次锁定的实际分配数量。 |
| `dimension_grp` | VARCHAR | Nullable | 消纳维度组。 |
| `available_date` | DATE | Nullable | 库存实际可用日期。 |

### 12. 微观有限产能排产表 (`ipc_detailed_schedule_ledger`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `sr_id` | VARCHAR | 🔑 PK | 工单单号 / 生产批次。 |
| `work_center` | VARCHAR | 🔑 PK | 物理工作中心代码。 |
| `sequence` | INTEGER | PK | 加工流水顺序号。 |
| `scheduled_start_day` | INTEGER | Nullable | 精密排程最早开工天偏移。 |
| `scheduled_finish_day` | INTEGER | Nullable | 精密排程完工天偏移。 |
| `run_time` | DOUBLE | Nullable | 纯加工耗时。 |
| `setup_time` | DOUBLE | Nullable | 考虑 Setup Matrix 的换模切换耗时（豁免为 0.0）。 |
| `qty` | DOUBLE | Nullable | 排产加工数量。 |
| `delay_days` | INTEGER | Nullable | 实际延期天数。 |
| `delay_penalty` | DOUBLE | Nullable | 延期阻抗惩罚。 |

### 13. 车间拉料配送表 (`ipc_line_call_request`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `call_id` | VARCHAR | 🔑 PK | 唯一拉料申请号。 |
| `parent_sr_id` | VARCHAR | Nullable | 父生产工单编码。 |
| `component_part` | VARCHAR | Nullable | 零部件物料 SKU。 |
| `site` | VARCHAR | Nullable | 配送站点代码。 |
| `required_qty` | DOUBLE | Nullable | 所需用量。 |
| `allocated_qty` | DOUBLE | Nullable | 实到拨付数量。 |
| `call_day` | INTEGER | Nullable | 期望配送天偏移。 |
| `status` | VARCHAR | Nullable | 配送状态（如 `Delivered`）。 |

### 14. 车间 BOM 齐套监控表 (`ipc_bom_kitting_status`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `parent_sr_id` | VARCHAR | 🔑 PK | 工单单号。 |
| `part_code` | VARCHAR | Nullable | 成品 SKU 编码。 |
| `required_date` | DATE | Nullable | 齐套截至时间。 |
| `total_components` | INTEGER | Nullable | BOM 展开所需总零部件数。 |
| `fulfilled_components` | INTEGER | Nullable | 备妥的零部件数量。 |
| `kitting_rate` | DOUBLE | Nullable | 齐套比率（自动计算列：`fulfilled_components / total_components`）。 |
| `kitting_status` | VARCHAR | Nullable | 齐套状态（自动计算列：`GREEN`、`YELLOW`、`RED`）。 |

### 15. 跨厂区置换调拨表 (`ipc_swap_result`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `demand_code` | VARCHAR | 🔑 PK | 被匹配满足的需求单号。 |
| `from_part` | VARCHAR | Nullable | 发货站点物料。 |
| `to_part` | VARCHAR | Nullable | 接收站点物料。 |
| `swapped_qty` | DOUBLE | Nullable | 置换调拨数量。 |
| `day` | INTEGER | Nullable | 调拨执行天。 |
| `alt_group` | VARCHAR | Nullable | 替代组编码。 |
| `swap_reason` | VARCHAR | Nullable | 置换原因（如 `Incomplete Substitution Stagnant Inventory SWAP`）。 |

### 16. 本地替代分配表 (`ipc_alternate_allocation`)

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `main_part` | VARCHAR | 🔑 PK | 父装配使用的主要部件。 |
| `alt_part` | VARCHAR | 🔑 PK | 实际被选用的替代部件。 |
| `allocated_qty` | DOUBLE | Nullable | 替代分配数量。 |
| `day` | INTEGER | Nullable | 分配执行天。 |
| `alt_class` | INTEGER | Nullable | 替代级别（1-Class 1，2-Class 2，3-Class 3）。 |

### 17. ETO 项目大盘定义表 (`ipc_project` / `ipc_project_wbs`)

* **`ipc_project`（项目主数据表）**：
| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `project` | VARCHAR | 🔑 PK | 项目编码。 |
| `customer` | VARCHAR | Nullable | 绑定的战略客户。 |
| `bonus_date` | VARCHAR | Nullable | 商业激励计划基准天偏移。 |
| `bonus_plan` | VARCHAR | Nullable | 绑定奖金计划名称。 |
| `penalty_date` | VARCHAR | Nullable | 逾期罚金计算基准天偏移。 |
| `penalty_plan` | VARCHAR | Nullable | 绑定罚金计划名称。 |
| `finish_date` | VARCHAR | Nullable | 预计交付天。 |
| `calc_finish_day` | INTEGER | Nullable | 计算出的 WBS 完工天。 |
| `delay_days` | INTEGER | Nullable | 逾期天数。 |
| `bonus_amount` | DOUBLE | Nullable | 结算奖金。 |
| `penalty_amount` | DOUBLE | Nullable | 结算罚款。 |
| `net_project_value` | DOUBLE | Nullable | 最终预计结算价值（`BaseValue + Bonus - Penalty`）。 |

* **`ipc_project_wbs`（WBS任务明细表）**：
| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `wbs_code` | VARCHAR | 🔑 PK | WBS任务节点的唯一层级编码。 |
| `project_code` | VARCHAR | PK | 项目编码，关联的 ETO 项目ID。 |
| `parent_wbs_code` | VARCHAR | Nullable | 父 WBS 任务节点编码。 |
| `wbs_level` | INTEGER | Nullable | WBS 任务深度层级。 |
| `wbs_status` | VARCHAR | Nullable | 任务状态：`ACTIVE`、`COMPLETED`、`PENDING`。 |
| `duration` | DOUBLE | Nullable | 任务计划加工耗时（天）。 |
| `early_start` | INTEGER | Nullable | 早开工天（正向计算）。 |
| `early_finish` | INTEGER | Nullable | 早完工天（$EarlyStart + Duration$）。 |
| `late_start` | INTEGER | Nullable | 迟开工天（逆向计算）。 |
| `late_finish` | INTEGER | Nullable | 迟完工天（$LateStart + Duration$）。 |

---

## 🧮 三、 C++ 求解器 13 阶段物理执行流与核心 SCM 算法深度推演

本章节详尽剖析 C++ 求解器在运行时的 13 个时序执行步骤，揭示数据模型如何在内存中流转并接受业务公式判定。

### 1. 第一阶段：前置算法断言校验 (run_patent_verification_tests)
* **业务目的**：在加载物理数据前，在纯内存中运行专利算子的单元测试。若发现数学公式执行有任何偏差，立刻熔断阻断程序（`abort()`），防止底层消纳和替换逻辑失真。
* **业务算法与公式**：
  * **一类替代算法（偏离度最小化）**：寻找能将各替代物料历史分配偏离度降至最低的零件 $i$ 优先满足：
    $$ Choice = \arg\max_{i \in \mathcal{G}} |H_i - Due_i| $$
    其中历史理论配额目标 $Due_i = (Total\_Hist + net\_demand) \times TargetRatio_i$，$H_i$ 为该替代物料历史累计已分配量 `alt_todate_qty`，$\mathcal{G}$ 为本组所有有效替代件集合。如果出现平局，选择 $TargetRatio_i$ 较大者。
  * **二类替代算法（供应商评级优先）**：基于供应商/配额评级最低者优先路由，维持采购配额比例稳定：
    $$ Choice = \arg\min_{i \in \mathcal{G}} \frac{H_i}{TargetRatio_i} $$
  * **三类替代算法（批量起订量与动态重新归一化）**：在多轮分配中，对每一轮 theoretical share $Due_i = CurrentRatio_i \times remaining\_net$ 最大的零件执行 lot_size 批量向上取整扣减：
    $$ ActualQty_{chosen} = \min\left( OnHand_{chosen}, \left\lceil \frac{Due_{chosen}}{LotSize_{chosen}} \right\rceil \times LotSize_{chosen} \right) $$
    扣减后，将该零件移出本轮候选集，对剩余零件的配额比例进行**动态重新归一化**更新：
    $$ CurrentRatio_{cand} = \frac{Due_{cand}}{\sum_{j \in \text{active}} Due_j} $$
  * **几何消纳算法**：时序数轴区间无分支交集覆盖计算，消除 if-else 控制流分支，实现极速净算：
    $$ consumed = \max\Big(0.0, \min(CD_{end}, CS_{start}) - CD_{start}\Big) $$
    $$ shortage = \max\Big(0.0, CD_{end} - \max(CD_{start}, CS_{start})\Big) $$
    其中 $[CD_{start}, CD_{end}]$ 为需求累计前缀和区间，$[0, CS_{start}]$ 为在手及在途供应前缀和区间。

### 2. 第二阶段：计划上下文与数据库初始化
* **业务目的**：根据命令行参数建立 DuckDB 连接。
* **逻辑控制**：若 `DEBUG_PERSIST == true`，则打开物理 DuckDB 文件连接，允许计划人员通过 DBeaver 运行 SQL 对账；若为 `false`，则开启纯内存沙盒（In-Memory Sandbox）模式，用于 What-If 多沙箱对比，不读写磁盘，彻底隔离脏数据。

### 3. 第三阶段：内存容器载体开辟
* **业务目的**：在内存中创建扁平的数据列式数据模型容器，接收从 DuckDB 加载的输入数据。

### 4. 第四阶段：数据库表结构迁移与空库数据注入
* **业务目的**：维护数据库物理表的一致性。
* **业务算法**：检查历史表名，自动转换 RENAME 为规范的 199 逻辑数据库模型。如果检测到数据库为空，则调用 `generate_massive_mock_data` 初始化百万级需求和多级 BOM 树并注入。

### 5. 第五阶段：层级确权与 Allotment 配额防波堤加载
* **业务目的**：解析 Product Family、Customer Group、Region 层次关系，穿透透传至 demands。
* **业务算法**：
  * **配额自动聚合**：若本场景下 `ipc_allotment_constraint` 为空，系统自动以 SQL 聚合现存的历史分配，按各物料/站点/日期初始化战术配额，限制后续订单无节制抢占战略配额：
    $$ AggregatedLimit = \sum_{psa \in \mathcal{P}} AssignedQty_{psa} $$
  * **配额加载**：加载 `allotment_constraints` 哈希表，锁定 `is_locked = true` 的刚性确权指标。

### 6. 第六阶段：IBP 预测层级分解与 Holt-Winters 时序推演
* **业务目的**：将宏观销售共识自顶向下分解，并推演未来需求趋势。
* **业务算法**：
  * **比例分解 (Proportional Disaggregation)**：将产品家族级需求 $Qty_{family}$，按历史独立需求比例分摊至底端 SKU 和 Customer：
    $$ Qty_{part, customer} = Qty_{family} \times \frac{Weight_{part, customer}}{\sum Weight} $$
    其中，分摊权重优先从 `ipc_dimension_grouping` 重写比率读取。若无重写，使用该物料-客户对的历史累计需求量作为权重。
  * **Holt-Winters 三重指数平滑推演 (加法模型)**：对历史需求时序 $X_t$ 推演点预测 $\hat{X}_{n+h}$ 并计算残差标准差 $\sigma_{error}$：
    - **初始化** (对于季节周期 $L_s$):
      $$ L_0 = \frac{1}{L_s} \sum_{i=1}^{L_s} X_i, \quad T_0 = \frac{1}{L_s^2} \sum_{i=1}^{L_s} (X_{L_s + i} - X_i), \quad S_i = X_i - L_0 \quad (i=1\ldots L_s) $$
    - **递推迭代** ($t=1\ldots n$):
      $$ L_t = \alpha (X_t - S_{t-L_s}) + (1-\alpha)(L_{t-1} + T_{t-1}) \quad (\alpha=0.2) $$
      $$ T_t = \beta (L_t - L_{t-1}) + (1-\beta) T_{t-1} \quad (\beta=0.1) $$
      $$ S_t = \gamma (X_t - L_t) + (1-\gamma) S_{t-L_s} \quad (\gamma=0.3) $$
    - **预测公式** (向前预测 $h$ 步):
      $$ \hat{X}_{n+h} = \max\Big(0.0, L_n + h \cdot T_n + S_{n + h - L_s \cdot \lfloor (h-1)/L_s \rfloor - L_s}\Big) $$
    - **残差标准差** (用于安全库存核算):
      $$ \sigma_{error} = \sqrt{\frac{1}{n} \sum_{t=1}^n \left(X_t - (L_{t-1} + T_{t-1} + S_{t-L_s})\right)^2} $$

### 7. 第七阶段：MEIO 多级安全库存传导与计算
* **业务目的**：在供应链图网络中，将成品端的需求波动风险沿 BOM 级联向下松弛传导，计算全网各节点时序安全库存（Safety Stock）。
* **业务算法**：
  * **级联需求与方差传播**（基于低层码 LLC 级联向下更新）：
    $$ D_{child} \leftarrow D_{child} + D_{parent} \times Factor $$
    $$ Var_{child} \leftarrow Var_{child} + Var_{parent} \times Factor^2 $$
    其中，$Factor = PerQty \times (1.0 + Scrap) \times TargetRatio$。
  * **动态安全库存求解**：
    利用求出的松弛需求均值 $D_i$ 和方差 $\sigma_{D, i}^2$（由 $Var_i$ 得到），求解在特定服务水平 $SL_i$（如 95%）下的安全库存目标：
    $$ SS_{i, t} = Z_i \times \sqrt{ L_i \cdot \sigma_{D, i, t}^2 + D_{i, t}^2 \cdot \sigma_{L, i}^2 } $$
    where $Z_i = \Phi^{-1}(SL_i)$ 为标准正态分布分位数（对于 95% 期望服务水平，$Z_i = 1.645$）。$L_i$ 为提前期，$\sigma_{L, i}^2$ 为前置期波动方差。计算出的 $SS$ 实时写回物料节点 `parts[i].safety_stock`。

### 8. 第八阶段：LSC Trees 全局供应链树展开与解耦点锚定
* **业务目的**：为所有独立需求建立全局物料展开网络树，并在图遍历中处理时序 ECN 切割与产品规格维度校验。
* **业务算法**：
  * **ECN 生效期裁定**：检验需求日期 $day$ 是否满足 BOM 条目生效期 $[eff\_start\_day, eff\_end\_day]$。如果不满足，图路径裁断、剔除该 BOM 展开路径。
  * **维度评估与产品规格降级**：
    支持联副产品维度规划。调用维度匹配关系。若订单需求规格值为 $OrderVal$，BOM 关系要求规格为 $BomVal$，且操作符为 $Op$（如 `GE` 大等于，即 512MB RAM 大等于 256MB 需求）：
    $$ evaluate\_dimension(OrderVal, Op, BomVal) = \text{True} $$
    如果返回 `False`，该 BOM 展开路径被截断。
  * **联副产品批量规划（Coproduct Recipe Batching）**：
    针对晶圆等切片过程中的联副产品产出，按照 recipe 规定的 batch_size and 产出比例（如 `ratio_512` = 0.5, `ratio_256` = 0.3, `ratio_128` = 0.2）进行投料排产。支持降级匹配策略（如 512MB 晶圆可消纳 256MB 订单需求），在消纳时，根据 `downbinning_priority`（如 `EXACT_FIRST` 或 `HIGHER_FIRST`）排序，并计算 leftover 留存在手。

### 9. 第九阶段：全局物料低层码 (LLC) 编译与依赖死锁检测
* **业务目的**：计算供应链全网物料的计算优先级（LLC 码），确保 MRP 进行物料爆炸时，“永远先计算父件，再计算子件”，避免需求遗漏，并检测非法循环依赖。
* **业务算法**：
  * **Bellman-Ford LLC 松弛**：
    $$ LLC(C) = \max \Big( LLC(C), LLC(P) + 1 \Big) \quad \forall (P \rightarrow C) \in BOM $$
  * **死锁熔断**：若松弛迭代轮数突破上限 100，判断为排产数据中存在循环 BOM 死锁依赖，直接抛出异常中断运行，强制阻断计算：
    `[致命死锁] BOM 拓扑编译中检测到闭环死锁依赖环路！系统强行熔断。`

### 10. 第十阶段：LBL-MRP 净需求时序消纳与级联爆炸
* **业务目的**：以 LLC 从高到低（即物料树自顶向下）的顺序，处理时序库存消纳、寻找 Class 1/2/3 替代料与跨厂区 Swap 置换，生成 Planned Orders 并级联分解至子件。
* **物理级并消纳（DSU 编组）**：
  在 LBL 净算中，为了实现多物料多站点的并发消纳，引擎在内存中对具有替代组（`alt_grp`）关联关系的物料在每一级 LLC 上进行 **DSU（Disjoint Set Union，并查集）并编组**，建立局部物料联通块，实现在联通块外部的安全并行多线程消纳。
* **消纳网格与消纳公式**：
  在手库存 $OnHand$ 与在途 Scheduled Receipts $SR$ 优先满足时序先到达的需求。
  - **安全库存与天数供应（DOS）防护**：
    天数供应（DOS）策略下，当期安全库存目标为未来 $N$ 天毛需求的累计和：
    $$ SS_{target} = \sum_{d = day + 1}^{day + \lceil intervals \rceil} GrossDemand_d $$
    现有可用库存一旦低于安全库存目标 $SS_{target}$，则强制拉起净缺口，防止库存被过度击穿：
    $$ net\_demand_{new} = net\_demand + \max\Big(0.0, SS_{target} - AvailableInventory\Big) $$
  - **Class 1/2/3 替换料计算**：若发生净缺口 $net\_demand > 0$，且物料属于某一替代组，根据 BOM 配置的 `priority` 分类执行第一阶段的 `allocate_class1/2/3` 配额平衡算法，从替代料可用库存中进行物料扣减。
  - **跨厂区 Swap 呆滞料自愈调拨**：若本料及本地替代料扣减后仍有缺口，且 BOM 关系设为 `soft`，Swap 引擎在其他站点寻找替代件。为保证安全性，其他站点只能调拨其超出安全库存的“呆滞部分”：
    $$ MaxSwapQty = \max\Big(0.0, OnHand_{alt} - SS_{alt}\Big) $$
  - **提前期拉伸与 Planned Order 生成**：
    加工提前期随订单批量动态拉伸：
    $$ LT_{total} = LeadTime_{fixed} + OrderQty \times RunRate $$
    依据工厂日历进行倒排排产（避开非工作日），确定计划订单开工时间 `start_day`：
    $$ start\_day = get\_workday\_offset\_backward(ev.day, LT_{total}, Calendar) $$
    批量规格化向上取整（Lot-Size 限制）：
    $$ OrderQty = \left\lceil \frac{net\_demand}{LotSize} \right\rceil \times LotSize $$
  - **级联需求爆炸**：
    $$ child\_gross = OrderQty \times PerQty \times (1.0 + Scrap) $$
    将该需求在子件的 `start_day` 注入其 3D DOD 时空密铺消纳矩阵 `gross_demand[part_id][day][dim_idx]` 中。

### 11. 第十一阶段：供需分配 Pegging 确权钉结
* **业务目的**：在内存中建立毛需求事件（Demands）与实体供应源（在手 On-Hand、在途 SR 或生成的 Planned Orders）之间的一对多或多对多物理绑定（Firm Pegging），记录至 `ipc_supply_assignment` 对账单中，防止优先级低的客户插单抢占战略供应。

### 12. 第十二阶段：双专利 MCDS DBD 有限能力精密排程调度
* **业务目的**：在 ITP 阶段执行产能瓶颈 Fair-Share 比例缩水平衡；在 IOP 阶段进行微观工序排程、 Setup Waiver 换模时间豁免与时序 ATP 递归回溯和 O(1) 事务回退。
* **业务算法与公式**：
  * **64位 Composite Priority 二进制优先级权重编码**：
    为了使所有子零件在多级装配中保持战略一致，子件通过 LLC 传导直接穿透继承父件 of 64 位权重：
    $$ U_{pri} = (committed\_val \ll 62) | (tier\_val \ll 60) | (due\_day \ll 44) | (priority \ll 28) | rev\_val $$
    其中：
    - `committed_val`：已确权工单为 0，敞口订单为 1；
    - `tier_val`：VVIP 客户为 0，普通客户按等级为 1、2、3；
    - `due_val`：交期天数；
    - `rev_val`：反转销售金额，$R_{max} - \min(R_{max}, \lfloor Revenue \rfloor)$。
  * **ITP 战术大盘 Fair-Share 产能比例平衡（仅 itp 模式运行）**：
    当瓶颈资源 $c$ 的总 Planned Orders 负荷过载时，系统优先拨付产能给 VVIP 订单（`tier_val == 0`）。非 VVIP 订单所能分配到的剩余产能，按缩放因子 $ScaleFactor_c$ 进行等比公平分摊：
    $$ satisfied\_vvip_c = \min(Load_{vvip, c}, AvailCap_c) $$
    $$ ScaleFactor_c = \frac{\max(0.0, AvailCap_c - satisfied\_vvip_c)}{Load_{non\_vvip, c}} $$
    若甚至 VVIP 负荷也超出可用产能，则 VVIP 订单也执行等比缩放，非 VVIP 订单分配量归零：
    $$ VVIPScaleFactor_c = \frac{AvailCap_c}{Load_{vvip, c}} $$
  * **TSP 贪婪派程算法与 Setup Matrix 阻抗计算**：
    车间精密调度使用基于 Setup 切换与延期违约的 TSP 启发式排产。在当前订单完成时，寻找阻抗值最低的下一个候选订单 $j$ 加工：
    $$ Impedance_j = SetupTime(current \rightarrow j) + TardinessPenalty(j) $$
    其中，预计耗时为 $ProjHours = AccHours + SetupTime + (Qty_j \times RunRate_j)$。延期天数 $DelayDays = \max(0, ProjDay - DueDay_j)$。延迟阻抗惩罚 $TardinessPenalty = DelayDays \times 1.5$。
  * **Setup Waiver 换模豁免算法**：
    订单加工耗费产能工时：
    $$ CapacityConsumption = \begin{cases} Qty \times RunTime + SetupTime & \text{若产品规格与前一单不同} \\ Qty \times RunTime & \text{若产品规格与前一单相同} \end{cases} $$
  * **CTP/ATP 递归回溯与 O(1) 零分配事务回滚**：
    在 `reserve_atp_and_capacity_recursive` 递归扣减物料和共享产能约束时，一旦失败，立刻触发事务回退，清理已分配的临时记录，回退所有层级的库存现有量变动，确保 $O(1)$ 时间内状态原样复原，防止 partial commitment 破坏供需一致性。

### 13. 第十三阶段：流式对账数据落盘与持久化同步
* **业务目的**：将内存计算产生的最终计划订单、替代调拨明细、微观排产工单，流式同步回写到物理 DuckDB 文件中，完成数据闭环。
* **业务操作**：利用 DuckDB 列式 Bulk Appender 流式大块追加，避免 SQL `INSERT` 循环执行的巨大 IO 瓶颈，实现分钟级的大盘数据自愈重算。

---

## 📅 四、 ETO 项目进度管理与 CPM 关键路径网络算法

针对项目型（Engineer-to-Order, ETO）供应链，系统集成物理 WBS 进度表与 CPM（Critical Path Method，关键路径法）网络求解，并在 IOP 阶段对合同迟延罚款及奖金进行对账：

### 1. CPM 关键路径正向与逆向推演
WBS 拓扑网络由任务节点 $j$（加工历时为 $Dur_j$）及依赖边构成。
* **正向计算（最早起止时间）**：从无紧前任务的根任务开始递推：
  $$ EarlyStart_j = \max_{p \in Pre(j)} EarlyFinish_p $$
  $$ EarlyFinish_j = EarlyStart_j + Dur_j $$
* **逆向计算（最迟起止时间）**：从项目最终合同交付期 $D_p$ （`delivery_lead_time`）倒排递推：
  $$ LateFinish_j = \min_{s \in Succ(j)} LateStart_s $$
  $$ LateStart_j = LateFinish_j - Dur_j $$

### 2. 商业计划与结算净值核算
系统最终预计完工天偏移为 $calc\_finish\_day = \max(EarlyFinish_{leaves})$。结算净价值公式为：
$$ NetProjectValue = BaseValue + Bonus(calc\_finish\_day) - Penalty(calc\_finish\_day) $$
* **延迟与奖惩细则**：
  - **抢先奖金**：若 $calc\_finish\_day \le bonus\_day\_offset$，根据 $bonus\_plan$ 获取额外的 $Bonus = on\_time\_bonus$ 商业返还款。
  - **逾期罚金**：若 $calc\_finish\_day > penalty\_day\_offset$，逾期天数 $DelayDays = calc\_finish\_day - penalty\_day\_offset$。根据 $penalty\_plan$ 罚金标准计算 $Penalty = DelayDays \times on\_time\_cost$ 并扣减。

---

## 🎬 五、 八大核心业务场景的 SCM 物理消纳数据流与对账审计

为了验证物理引擎在处理多站点多级装配高频调整时的数学严密性，以下针对八个核心业务场景进行底层账本数据的 side-by-side 对账审计。

### 场景 1：一类替换件（偏离度最小化替换）
* **场景说明**：为保证主件在 SITE_001 生产时能按 60% : 40% 的刚性比例分摊至替代件 `P_COMP_A` 与 `P_COMP_B`。历史已分配量为 0.0。连续下达三轮需求：10 件、10 件、20 件。
* **物理输入状态（BOM Item 表）**：
```sql
-- bomid, site, component, alt_grp, priority, target, alt_todate_qty
('BOM_FG_01', 'SITE_001', 'P_COMP_A', 'ALT_GRP_01', 1, 0.60, 0.0),
('BOM_FG_01', 'SITE_001', 'P_COMP_B', 'ALT_GRP_01', 1, 0.40, 0.0)
```
* **计算轨迹与对账结果**：
  - **第一轮分配（净需求 10）**：计算理论比率 $Due_A = 6.0$, $Due_B = 4.0$。偏离值 $|0-6|=6.0 > |0-4|=4.0$，分配给 `P_COMP_A` 10 件。累计：A(10.0), B(0.0)。
  - **第二轮分配（净需求 10）**：累计总需求 20。计算理论比率 $Due_A = 12.0$, $Due_B = 8.0$。偏离值 $|10-12|=2.0 < |0-8|=8.0$，分配给 `P_COMP_B` 10 件。累计：A(10.0), B(10.0)。
  - **第三轮分配（净需求 20）**：累计总需求 40。计算理论比率 $Due_A = 24.0$, $Due_B = 16.0$。偏离值 $|10-24|=14.0 > |10-16|=6.0$，分配给 `P_COMP_A` 20 件。累计：A(30.0), B(10.0)。
* **物理对账单输出表 (`ipc_alternate_allocation`)**：
```sql
-- main_part, alt_part, allocated_qty, day, alt_class
('FG_BOM_COMP', 'P_COMP_A', 10.0, 5, 1),
('FG_BOM_COMP', 'P_COMP_B', 10.0, 15, 1),
('FG_BOM_COMP', 'P_COMP_A', 20.0, 25, 1)
```
* **审计对账 SQL**：
  ```sql
  SELECT alt_part, SUM(allocated_qty) AS total FROM ipc_alternate_allocation GROUP BY alt_part;
  -- 对账结果：P_COMP_A 占 75%，P_COMP_B 占 25%，比例与 60:40 的差值在离散批量下处于数学最优区间。
  ```

### 场景 2：二类替换件（供应商配额优先路由）
* **场景说明**：比率目标 P1(60%), P2(40%)。连续下达三轮需求：10件、20件、30件。二类替换基于供应商分配水平进行平稳路由。
* **物理输入状态（BOM Item 表）**：
```sql
('BOM_FG_02', 'SITE_001', 'P_SUP_A', 'ALT_GRP_02', 2, 0.60, 0.0),
('BOM_FG_02', 'SITE_001', 'P_SUP_B', 'ALT_GRP_02', 2, 0.40, 0.0)
```
* **计算轨迹与对账结果**：
  - **第一轮（需求10）**：均无历史。优先分配给比例大的 `P_SUP_A` 10 件。評级变为：A($10/0.6=16.67$), B($0/0.4=0.0$)。
  - **第二轮（需求20）**：`P_SUP_B` 评级为 0.0 最小，分配给 `P_SUP_B` 20 件。评级变为：A(16.67), B($20/0.4=50.0$)。
  - **第三轮（需求30）**：`P_SUP_A` 评级 16.67 较小，分配给 `P_SUP_A` 30 件。
* **物理对账单输出表 (`ipc_alternate_allocation`)**：
```sql
('P_MAIN_02', 'P_SUP_A', 10.0, 10, 2),
('P_MAIN_02', 'P_SUP_B', 20.0, 20, 2),
('P_MAIN_02', 'P_SUP_A', 30.0, 30, 2)
```
* **审计对账 SQL**：
  ```sql
  SELECT alt_part, SUM(allocated_qty) AS total FROM ipc_alternate_allocation GROUP BY alt_part;
  ```

### 场景 3：三类替换件（起订量批量与动态重新归一化）
* **场景说明**：配额比例 P1 50% (LotSize 20.0), P2 30% (LotSize 15.0), P3 20% (LotSize 10.0)。首期总需求 100 件。
* **物理输入状态（BOM Item 表）**：
```sql
('BOM_FG_03', 'SITE_001', 'P_LOT_1', 'ALT_GRP_03', 3, 0.50, 0.0, 20.0),
('BOM_FG_03', 'SITE_001', 'P_LOT_2', 'ALT_GRP_03', 3, 0.30, 0.0, 15.0),
('BOM_FG_03', 'SITE_001', 'P_LOT_3', 'ALT_GRP_03', 3, 0.20, 0.0, 10.0)
```
* **计算轨迹与对账结果**：
  - **第一步**：P_LOT_1 theoretical due 最大为 50 件。Lot-Size 规整向上取整：$\lceil 50/20 \rceil \times 20 = 60$ 件。剩余未满足缺口为 40 件。
  - **第二步**：由于 P_LOT_1 已足额，将其移出候选。对剩余 P_LOT_2 (30%) 与 P_LOT_3 (20%) 进行**动态重新归一化**更新，比率更新为 30/(30+20) = 0.6 和 20/(30+20) = 0.4。
  - **第三步**：剩余 40 件中，P_LOT_2 theoretical due = $40 \times 0.6 = 24$ 件。进行 Lot-Size 规整向上取整：$\lceil 24/15 \rceil \times 15 = 30$ 件。剩余未满足缺口 10 件。
  - **第四步**：分配给 P_LOT_3 剩余 10 件（恰好满足 Lot-Size 10.0）。
* **物理对账单输出表 (`ipc_alternate_allocation`)**：
```sql
('P_MAIN_03', 'P_LOT_1', 60.0, 1, 3),
('P_MAIN_03', 'P_LOT_2', 30.0, 1, 3),
('P_MAIN_03', 'P_LOT_3', 10.0, 1, 3)
```

### 场景 4：跨厂区 Swap 呆滞料调拨与安全库存保护
* **场景说明**：厂区 SITE_001 发生主件 `COMP_1` 严重缺货 70 件。替代件 `COMP_ALT` 在 SITE_002 在库 150 件，SITE_002 设定的动态安全库存水位为 100 件。BOM 关系设为 `soft`。
* **物理输入状态（OnHand 表与 Material Node 表）**：
```sql
-- OnHand
('LOC_002', 'COMP_ALT', 'SITE_002', '2026-05-29', 150.0, 'Standard')
-- Material Node
('COMP_ALT', 'SITE_002', 100.0) -- safety_stock = 100.0
```
* **计算轨迹与对账结果**：
  - 求解器检索 SITE_001 本地无料。触发跨厂区调拨机制。
  - 调拨引擎检索 SITE_002 的 `COMP_ALT` 库存，计算其在保障安全库存前提下的最高呆滞可调配量为：$150.0 - 100.0 = 50.0$ 件。
  - 调配出库 50 件 `COMP_ALT` 调拨给 SITE_001，剩余 20 件缺口触发计划订单采购补货，绝不击穿 SITE_002 的 100 件安全库存防护堤。
* **物理对账单输出表 (`ipc_swap_result`)**：
```sql
-- demand_code, from_part, to_part, swapped_qty, day, swap_reason
('DEMAND_SWAP_001', 'COMP_ALT@SITE_002', 'COMP_1@SITE_001', 50.0, 0, 'Incomplete Substitution Stagnant Inventory SWAP')
```
* **审计对账 SQL**：
  ```sql
  SELECT from_part, to_part, swapped_qty FROM ipc_swap_result;
  ```

### 场景 5：BOM ECN 切割与产品规格维度降级匹配
* **场景说明**：成品 A 生产需要大容量 RAM 规格 `DIM_102.0` (512MB RAM)。BOM 规定主料 `COMP_MAIN` 在第 10天切割失效（ECN），备选 `COMP_SUB` 允许通过 `GE` 大等于操作符兼容降级使用。下达两笔需求：D1（第 5 天），D2（第 12 天）。
* **物理输入状态（BOM Item 与 Grouping 规则表）**：
```sql
-- BOM Item
('BOM_A', 'SITE_001', 'COMP_MAIN', 1.0, 0.0, 'GRP_01', 1, 1.0, 0.0, 0.0, 0, 10, 'alt'),
('BOM_A', 'SITE_001', 'COMP_SUB', 1.0, 0.0, 'GRP_01', 2, 0.0, 0.0, 0.0, 10, 999, 'alt')
```
* **物理对账单输出表 (`ipc_planned_order` 与 `ipc_supply_assignment`)**：
  - 成功在 Day 5 生成对应 `COMP_MAIN` 的计划订单，在 Day 12 生成对应 `COMP_SUB` 的计划订单。
* **审计对账 SQL**：
  ```sql
  SELECT ipc_planned_order, part, qty, due_date FROM ipc_planned_order;
  ```

### 场景 6：ITP 战术大盘 VVIP 保护与 Fair-Share 产能比例平衡
* **场景说明**：瓶颈资源 `LINE_FINISHED` 额定负荷上限为 8.0 小时。
  - 订单 1 (VVIP)：工时负荷 6.0 小时。
  - 订单 2 (普通)：工时负荷 6.0 小时。
* **计算轨迹与对账结果**：
  - 总负荷 12.0 小时过载。求解器检测到 itp 模式。
  - 优先全额满足 VVIP 订单 1 拨付 6.0 小时，剩余可用容量 $8.0 - 6.0 = 2.0$ 小时。
  - 普通订单 2 分摊剩余 2.0 小时。计算等比缩放因子 $ScaleFactor = 2.0 / 6.0 = 0.3333$。订单 2 的生产数量从 100 件等比削减为 33 件。
* **物理对账单输出表 (`ipc_planned_order`)**：
```sql
-- planned_order_id, part, qty, due_date
('PO_VVIP_01', 'FG_A', 100.0, '2026-06-08'),
('PO_NORMAL_01', 'FG_A', 33.0, '2026-06-08')
```
* **审计对账 SQL**：
  ```sql
  SELECT part_code, order_qty FROM ipc_planned_order_ledger;
  ```

### 场景 7：Setup Waiver 换模工时豁免
* **场景说明**：工作中心换模准备时间 SetupTime = 1.0 小时，变动加工 RunRate = 0.05 小时/件。前一个工序排程规格为 `DIM_102.0`。当前待排 Planned Order 规格也是 `DIM_102.0`，批量为 100。
* **计算轨迹与对账结果**：
  - 求解器在第十二阶段派程调度当前 Planned Order。
  - 比对当前 Planned Order 的规格维度 `DIM_102.0` 与前一工单完全一致。
  - 触发 Setup Waiver，换模工时设为 0.0，只扣减工作中心 $100 \times 0.05 = 5.0$ 小时产能。
* **物理对账单输出表 (`ipc_detailed_schedule_ledger`)**：
```sql
-- sr_id, work_center, sequence, run_time, setup_time, qty
('SR_WO_901', 'WC_ASSEMBLY', 2, 5.0, 0.0, 100.0)
```
* **审计对账 SQL**：
  ```sql
  SELECT sr_id, setup_time, run_time FROM ipc_detailed_schedule_ledger;
  ```

### 场景 8：时序 ATP 预占失败与 O(1) 事务性回滚
* **场景说明**： Planned Order 申请扣减半成品零件 A 现有库存 50 件，系统已在 `ipc_onhand` 执行了扣减。但在递归向下预占子件共享产能容量时发现无法满足，导致整个计划订单宣告预占失败。
* **计算轨迹与对账结果**：
  - 求解器在 `reserve_atp_and_capacity_recursive` 阶段发现产能越界。
  - 立刻触发 `AllotmentRollbackGuard` 事务隔离栈，在内存中反向执行逆向操作，把已扣减的半成品零件 A 库存加回还原，清除脏临时订单。
* **物理对账单输出表**：
  - 各对账表无本 Planned Order 记录，`ipc_detailed_schedule_ledger` 中未生成本工单的任何脏痕迹，保持数据绝对一致。
* **审计对账 SQL**：
  ```sql
  SELECT * FROM ipc_detailed_schedule_ledger WHERE sr_id = 'SR_FAILED_001';
  ```

---
