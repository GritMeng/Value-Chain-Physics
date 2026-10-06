# IPC 智能计划与控制系统：计划工作簿与 What-If 多沙箱推演用户手册

本手册是 IPC (Intelligent Planning & Control) 求解器引擎的计划员实操与系统配置规范，其设计完全对标 **Kinaxis RapidResponse** 的“工作簿（Workbook）- 工作表（Sheet）”式交互与“What-If 多沙箱并发模拟”体系。

本手册旨在指导计划员如何通过审计底层 DuckDB 物理账本进行供需平衡管理，定义各工作表的输入与计算逻辑，并演示如何通过 What-If 修改在秒级内自愈净缺口并进行有限产能排产与 ETO 项目进度决策。

---

## 📐 一、 IPC 全息计划工作簿（Workbook）与表结构设计

在 IPC 体系中，计划员不直接操纵 C++ 内核，而是通过对账数据库的逻辑表（对应工作表 Sheet）进行审阅与重写。工作簿中的字段被明确划分为：
* **用户输入字段（Input Overrides）**：计划员可在前端网格或通过 SQL 手工改写、锁定的参数。
* **求解器计算字段（Calculated Fields）**：由 ITP/IOP 求解器在运行期通过物理算法自动冲减、传导算出的字段。

---

### 1. 物料站点控制工作表 (`ipc_material_node` / 对应数据库物理表 `ipc_material_node`)
> **业务说明**：定义物料在各站点厂区（Site）下的独立控制参数、安全库存规则与计划天数时界。

| 字段物理名 (FieldName) | 物理类型 (DataType) | 必填/键 (Key) | 详细业务备注与 SCM 规则 (Comment) |
| :--- | :--- | :--- | :--- |
| `part` | VARCHAR | 🔑 PK | 物料唯一编码 (SKU)。 |
| `site` | VARCHAR | 🔑 PK | 厂区站点代码 (如 SITE_001)。 |
| `description` | VARCHAR | Nullable | 物料中文详述。 |
| `is_phantom` | BOOLEAN | Nullable | 虚拟件标识。若为 `true`，直接穿透分解。 |
| `mrp_rule` | VARCHAR | Nullable | 净需求计算规则。默认值 `MRP`。 |
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
| `safety_stock_value` | DOUBLE | Nullable | 安全库存资本占用额度（计算列：`safety_stock * selling_ave_price`）。

* **示例数据**：
```sql
('FG_A', 'SITE_01', '成品A描述', false, 'MRP', 100.0, 5.0, 1, 0.05, 5.0, true, 'Standard', 2, 'Standard', 35.5, 0.0, 'Dynamic', 'DAYS_OF_SUPPLY', 5.0, 'DEFAULT', 3550.0)
```

---

### 2. 在手现有库存工作表 (`ipc_onhand`)
> **业务说明**：记录各仓库与站点下的物理现有量（OnHand）和可用日期。

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `location` | VARCHAR | 🔑 PK | 库房库位名称。 |
| `part` | VARCHAR | 🔑 PK | 对应的物料零件号。 |
| `site` | VARCHAR | 🔑 PK | 所属站点。 |
| `available_date` | DATE | PK | 库存可用日期。 |
| `qty` | DOUBLE | Nullable | 现有量库存数量（必须 $\ge 0.0$）。 |
| `inventory_type` | VARCHAR | Nullable | 库存质量分类。默认 `Standard`。 |

---

### 3. 在途订单供应工作表 (`ipc_scheduled_receipt`)
> **业务说明**：展示已经下达但尚未完工入库的在途采购单（PO）或生产工单（WO）。

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `sr_id` | VARCHAR | 🔑 PK | 唯一在途单号。 |
| `item` | DOUBLE | 🔑 PK | 行项明细号。 |
| `to_part` | VARCHAR | Nullable | 接收的物料。 |
| `qty` | DOUBLE | Nullable | 在途供应量。 |
| `to_site` | VARCHAR | Nullable | 接收站点。 |
| `request_due_date` | DATE | Nullable | 合同约定交付就绪日期。 |
| `supply_status` | VARCHAR | Nullable | 在途状态（如 `In-Transit`, `Shipped`, `Confirmed`）。若为 `Ignore` 则求解器忽略此在途。 |

---

### 4. BOM 关系明细工作表 (`ipc_bom_item`)
> **业务说明**：定义组件拓扑装配逻辑与替代规则。

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `bomid` | VARCHAR | 🔑 PK | BOM 配方编码。 |
| `site` | VARCHAR | 🔑 PK | 站点代码。 |
| `component` | VARCHAR | 🔑 PK | 子组件物料编码。 |
| `perqty` | DOUBLE | Nullable | 单件装配消耗量。默认 `1.0`。 |
| `scrap` | DOUBLE | Nullable | 装配损耗率（如 0.05 代表 5% 损耗）。 |
| `alt_grp` | VARCHAR | Nullable | 替代组编码。属于同组的组件物料属于可替换物料。 |
| `priority` | INTEGER | Nullable | 替代类型。`1` Class 1 偏离度最小；`2` Class 2 供应商评级优先；`3` Class 3 批量规整归一化。 |
| `target` | DOUBLE | Nullable | 替代分摊比率目标（如 0.60 代表期望分摊 60% 份额）。 |
| `alt_todate_qty` | DOUBLE | Nullable | 历史累计已替代分摊数量。默认 `0.0`。 |
| `lot_size` | DOUBLE | Nullable | 替代发生时的起订批量限制。默认 `0.0`。 |
| `eff_start_day` | INTEGER | Nullable | ECN 生效开始的天偏移量。 |
| `eff_end_day` | INTEGER | Nullable | ECN 失效截至天偏移量。 |
| `ltb_limit` | DOUBLE | Nullable | Last-Time-Buy 呆滞料限额限制。 |
| `mix_group_id` | INTEGER | Nullable | 混合替代物料优先级组。 |
| `relationship_type` | VARCHAR | Nullable | 替代刚性。`alt` 硬替代（受ECN限制）；`soft` 软替代（允许Swap跨站点调拨）。 |

---

### 5. 工艺路线工作表 (`ipc_bom_route`)
> **业务说明**：定义成品的工艺路线与 BOM ID 映射。

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `site` | VARCHAR | 🔑 PK | 站点代码。 |
| `part` | VARCHAR | 🔑 PK | 成品装配 SKU 编码。 |
| `bomid` | VARCHAR | PK | 绑定的 BOM 编码。 |
| `priority` | INTEGER | Nullable | 工艺路线选择优先级。默认 `1`。 |
| `bom_type` | VARCHAR | Nullable | 配方类别。 |

---

### 6. 独立需求工作表 (`ipc_independent_demand`)
> **业务说明**：汇集大盘预测（Forecasts）与销售订单（Sales Orders）。

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

---

### 7. 战术确权配额控制表 (`ipc_allotment_constraint`)
> **业务说明**：ITP 战术层下发的防波堤配额限制参数。

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

---

### 8. 战术确权配额执行账本表 (`ipc_allotment_ledger`)
> **业务说明**：IOP 滚动排产中扣减配额后的实时剩余水位监控。

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

---

### 9. 建议计划订单工作表 (`ipc_planned_order`)
> **业务说明**：系统净需求爆炸后自动生成的建议补货订单列表。

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

---

### 10. 建议计划分配工作表 (`ipc_planned_supply_assignment`)
> **业务说明**：记录计划订单与销售订单/预测之间的供需钉结分配关系。

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
| `planned_order` | VARCHAR | Nullable | 关联的计划订单号。 |
| `part_ready_date` | DATE | Nullable | 最早物料齐套日期。 |

---

### 11. 锁定供需钉结分配表 (`ipc_supply_assignment`)
> **业务说明**：记录在手现有量或在途 SR 与独立需求之间的锁定钉结关系。

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

---

### 12. 微观有限产能排产表 (`ipc_detailed_schedule_ledger`)
> **业务说明**：工单在具体工作中心下的有时序排程流水记录。

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

---

### 13. 车间拉料配送表 (`ipc_line_call_request`)
> **业务说明**：记录车间向仓库发起的零部件拉料申请及配送水位。

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

---

### 14. 车间 BOM 齐套监控表 (`ipc_bom_kitting_status`)
> **业务说明**：实时展现排产工单的齐套状态，指导车间领料上线。

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `parent_sr_id` | VARCHAR | 🔑 PK | 工单单号。 |
| `part_code` | VARCHAR | Nullable | 成品 SKU 编码。 |
| `required_date` | DATE | Nullable | 齐套截至时间。 |
| `total_components` | INTEGER | Nullable | BOM 展开所需总零部件数。 |
| `fulfilled_components` | INTEGER | Nullable | 备妥的零部件数量。 |
| `kitting_rate` | DOUBLE | Nullable | 齐套比率（自动计算列：`fulfilled_components / total_components`）。 |
| `kitting_status` | VARCHAR | Nullable | 齐套状态（自动计算列：`GREEN`、`YELLOW`、`RED`）。 |

---

### 15. 跨厂区置换调拨表 (`ipc_swap_result`)
> **业务说明**：展现求解器执行跨厂区 Swap 调拨置换出来的明细和原因。

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `demand_code` | VARCHAR | 🔑 PK | 被匹配满足的需求单号。 |
| `from_part` | VARCHAR | Nullable | 发货站点物料。 |
| `to_part` | VARCHAR | Nullable | 接收站点物料。 |
| `swapped_qty` | DOUBLE | Nullable | 置换调拨数量。 |
| `day` | INTEGER | Nullable | 调拨执行天。 |
| `alt_group` | VARCHAR | Nullable | 替代组编码。 |
| `swap_reason` | VARCHAR | Nullable | 置换原因（如 `Incomplete Substitution Stagnant Inventory SWAP`）。 |

---

### 16. 本地替代分配表 (`ipc_alternate_allocation`)
> **业务说明**：呈现各物料在本地替代组中分摊的明细以及替代层级。

| 字段物理名 | 物理类型 | 必填/键 | 详细业务备注 |
| :--- | :--- | :--- | :--- |
| `main_part` | VARCHAR | 🔑 PK | 父装配使用的主要部件。 |
| `alt_part` | VARCHAR | 🔑 PK | 实际被选用的替代部件。 |
| `allocated_qty` | DOUBLE | Nullable | 替代分配数量。 |
| `day` | INTEGER | Nullable | 分配执行天。 |
| `alt_class` | INTEGER | Nullable | 替代级别（1-Class 1，2-Class 2，3-Class 3）。 |

---

### 17. ETO 项目协同工作簿表 (`ipc_project` 与 `ipc_project_wbs`)
> **业务说明**：针对项目型制造（ETO）管理进度 WBS 拓扑网络、结算净值与商业计划。

* **`ipc_project`（项目定义主表）**：
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

## 🔄 二、 What-If 多沙箱推演与重算工作流

在 IPC 系统中，当面临大盘需求异动、供货中断或工作中心容量受阻时，计划员可以发起 **What-If 沙箱推演工作流**，其底层物理机制是克隆一个纯内存的 DuckDB 沙盒场景并对账，杜绝将脏数据持久化到主 Baseline 中。

### 1. 沙箱隔离克隆机制 (Scenario Creation & Cloning)
当计划员在控制台创建一个名为 `sandbox_sim_01` 的沙盒时，后台执行的 SQL 逻辑为克隆指定 Baseline 版本的参数数据：
```sql
-- 1. 创建沙盒记录
INSERT INTO ipc_collab_scenario (scenario_code, scenario_name, created_by, status)
VALUES ('sandbox_sim_01', '模拟晶圆组件SITE_01缺料置换', 'PLANNER_09', 'Draft');

-- 2. 在配额限制表上克隆战术配额防波堤，以便计划员手工 overrides 调整
INSERT INTO ipc_allotment_constraint (scenario_id, part_code, site_code, region, customer_group, product_family, day, itp_calculated_qty, override_qty, is_locked)
SELECT 'sandbox_sim_01', part_code, site_code, region, customer_group, product_family, day, itp_calculated_qty, itp_calculated_qty, false
FROM ipc_allotment_constraint
WHERE scenario_id = 'scenario_baseline';
```

### 2. 实操演示：调整物料 override 字段重算自愈
假设 SITE_001 出现 200 件物料缺口，计划员可以通过 DBeaver 执行 Overrides 临时参数重写：
```sql
-- 重写 SITE_01 下 FG_A 的 DOS 安全覆盖天数，使其从 5 天临时提升至 10 天，加大供应缓冲
UPDATE ipc_material_node
SET dos_intervals = 10.0, ss_rule = 'Dynamic', dos_policy = 'DAYS_OF_SUPPLY'
WHERE part = 'FG_A' AND site = 'SITE_01';

-- 释放 SITE_02 下组件的替代锁定，允许跨厂区 Swap 调拨
UPDATE ipc_bom_item
SET relationship_type = 'soft'
WHERE component = 'COMP_ALT' AND site = 'SITE_02';
```
更新输入参数后，计划员在网格界面中点击 **“一键重算 (Solve Scenario)”**，前台进程将以命令行触发 C++ 求解器执行：
```bash
./ipc_solver_engine --db data/ipc.db --mode iop --scenario sandbox_sim_01
```
求解器在 100 毫秒内读入内存，流式运行 13 个阶段计划引擎：
1. 校验 Class 1/2/3 替代平衡算子；
2. 级联重算 `parts` 安全库存目标水位；
3. 执行 DSU 并查集并消纳；
4. 调用 TSP 换模豁免及 ATP 时序回退；
5. 流式 Bulk Append 将计划对账数据写回 DuckDB 数据库。

### 3. 多沙盒 KPI 对账看板 (Scenario Reconcile & Cockpit)
重算完成后，计划员可在仪表盘（Worksheets Cockpit）对比两个场景的财务与供应损益。
* **Scenario A (Baseline)**: OTD 75.0%, CarryCost 12,000元, Delayed Project Value -50,000元。
* **Scenario B (sandbox_sim_01)**: OTD 98.2%, CarryCost 15,500元, Project Net Value 180,000元（未延期且获得抢先奖金）。
* **决策动作**：计划员审阅认为 Scenario B 的项目结算效益远超库存持有成本。在控制台点击 **“推送到 Baseline (Promote to Active)”**，系统执行 SQL 回写：
```sql
-- 将沙盒方案在 allotment 确权上的 overrides 同步更新至主 Baseline 中
UPDATE ipc_allotment_constraint AS dest
SET override_qty = src.override_qty, is_locked = true
FROM ipc_allotment_constraint AS src
WHERE dest.scenario_id = 'scenario_baseline' 
  AND src.scenario_id = 'sandbox_sim_01'
  AND dest.part_code = src.part_code 
  AND dest.site_code = src.site_code
  AND dest.day = src.day;
```

---

## 📈 三、 SCM 核心计划指标（KPIs）核算公式

求解器根据 DuckDB 产出的对账账本，使用 LaTeX 数学定义全网 SCM KPIs：

### 1. 订单及时交付率 (On-Time Delivery, OTD)
衡量客户订单交期满足率的终极计划指标。
$$ OTD = \frac{\sum_{d \in \mathcal{D}_{open}} \mathbb{I}\big( calc\_finish\_day_d \le request\_due\_date_d \big) \cdot Revenue_d }{\sum_{d \in \mathcal{D}_{open}} Revenue_d} \times 100\% $$
其中，$\mathcal{D}_{open}$ 为当前未关闭订单项目，$\mathbb{I}(\cdot)$ 为指示函数（交期小于等于要求交期为 1，否则为 0），$Revenue_d$ 为订单预计金额。

### 2. 计划订单配额阻断率 (Blocked Demand Ratio)
评估因为战术配额（Allotment limit）耗尽，导致中高优需求无法承诺补货的比率。
$$ BlockedRatio = \frac{\sum_{al \in \mathcal{AL}} blocked\_demand\_qty_{al}}{\sum_{al \in \mathcal{AL}} \big( allotment\_limit_{al} + blocked\_demand\_qty_{al} \big)} \times 100\% $$
其中，$\mathcal{AL}$ 为全网所有 Allotment Ledger 明细项集合。

### 3. 库存持有资本价值 (Inventory Carrying Value, ICV)
度量大盘库存资本占用金额，对齐财务 ROIC 计算。
$$ ICV = \sum_{node \in \mathcal{M}} safety\_stock_{node} \times selling\_ave\_price_{node} $$
其中，$\mathcal{M}$ 为 Material Node 物料站点表集合，安全库存越高，ICV 越高。

### 4. 工作中心产能过载率 (Capacity Overload Ratio)
诊断工作中心的物理负荷风险。
$$ CapacityOverload_c = \frac{\sum_{s \in \mathcal{S}_c} \big( run\_time_s + setup\_time_s \big) \cdot qty_s }{AvailableCapacity_c} \times 100\% $$
其中，$\mathcal{S}_c$ 为该排程周期内指派给工作中心 $c$ 的全部工单记录。

---

## 🔍 四、 计划追溯：供需 Pegging 确权与缺料瓶颈审计

当大盘 OTD 恶化或项目 WBS CPM 发生逾期时，计划员无需人肉查找，而是顺着供需 Pegging 钉结账本，执行 **4 步 SQL 级联溯源**，穿透定位到位于最底层的特定零部件在特定工作中心的产能卡脖子瓶颈。

### 1. 第一步：定位延期关键需求
查出所有未按时承诺（实际承诺交期 AvailableDate 大于期望交期 RequestDueDate）的 VVIP 级独立需求订单：
```sql
SELECT demand, part, request_qty, request_due_date, available_date, (available_date - request_due_date) AS delay_days
FROM ipc_independent_demand
WHERE status = 'Open' AND customer_tier = 0 AND available_date > request_due_date
ORDER BY delay_days DESC;
-- 假定锁定到延期需求单号：'DEMAND_00120'
```

### 2. 第二步：追踪该需求的未足额供应源
查询 Pegging 表 `ipc_planned_supply_assignment`，找出为满足该需求生成的计划补货订单（Planned Order）：
```sql
SELECT psa.supply, psa.part, psa.assigned_qty, psa.available_date, psa.part_ready_date
FROM ipc_planned_supply_assignment AS psa
WHERE psa.demand = 'DEMAND_00120' AND psa.supply_type = 'PlannedOrder'
ORDER BY psa.available_date DESC;
-- 假定捕获到未满足的计划订单号：'PO_SIM_0099'
```

### 3. 第三步：追踪计划订单齐套中的缺料零部件
因为齐套就绪日期 `part_ready_date` 滞后，查询拉料明细与齐套监控表，定位是哪个零部件导致了工单齐套延误：
```sql
SELECT ks.parent_sr_id, ks.part_code, ks.kitting_rate, ks.kitting_status, lcr.component_part, lcr.required_qty, lcr.allocated_qty
FROM ipc_bom_kitting_status AS ks
JOIN ipc_line_call_request AS lcr ON ks.parent_sr_id = lcr.parent_sr_id
WHERE lcr.parent_sr_id = 'PO_SIM_0099' AND ks.kitting_status = 'SHORTAGE'
  AND lcr.allocated_qty < lcr.required_qty;
-- 假定定位到缺料零部件 SKU 为：'COMP_ALT@SITE_01'
```

### 4. 第四步：穿透到产能卡脖子瓶颈工作中心
零部件 COMP_ALT 没有足够的在手机制，说明其生产工单在车间级被排程推迟。关联微观排产表，确定是哪个工作中心的产能负荷超载，且因为没有触发 Setup Waiver（频繁换模）而产生延期：
```sql
SELECT dsl.work_center, dsl.sr_id, dsl.sequence, dsl.scheduled_start_day, dsl.scheduled_finish_day, dsl.setup_time, dsl.delay_days
FROM ipc_detailed_schedule_ledger AS dsl
WHERE dsl.sr_id IN (
    SELECT sr_id FROM ipc_detailed_schedule_ledger 
    WHERE part = 'COMP_ALT' AND site = 'SITE_01'
) AND dsl.delay_days > 0
ORDER BY dsl.scheduled_finish_day DESC;
-- 穿透审计结果：COMP_ALT的工单SR_9901在工作中心 'LINE_SEMI' 的第 3工序发生延误。
```
