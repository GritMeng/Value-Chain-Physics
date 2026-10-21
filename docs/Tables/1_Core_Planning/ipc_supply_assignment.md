---
table_name: "ipc_supply_assignment"
alias: "supply_assignment"
module: "1_Core_Planning"
cpp_struct: "SupplyAssignmentRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 1_core_planning
---

# 🏷️ `ipc_supply_assignment` (supply_assignment)

> **业务说明**: 锁定/确权供应钉结分配表（Firm Pegging）。记录在途、在库供应与需求之间的锁定配额钉结关系，包含我们的 ITP/IOP Allotment 配额防波堤划转明细，防止普通订单抢占战略配额。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `assigned_qty` | assigned_qty | `DECIMAL(18,2)` | PK / NOT NULL | 本次配额锁定的实际分配数量 |
| `assigned_parent_qty` | assigned_parent_qty | `DECIMAL(18,2)` | Nullable | 此supply分配的直接上层的数量 |
| `request_qty` | request_qty | `DECIMAL(18,2)` | Nullable | 原始请求的订货/预测数量 |
| `required_parent_qty` | required_parent_qty | `DECIMAL(18,2)` | Nullable | 此supply上层需求的数量 |
| `demand` | demand | `VARCHAR(18)` | 🔑 **PK / Required** | 需求编号 |
| `item` | item | `DOUBLE` | PK / NOT NULL | 需求行项目号 |
| `ind_part` | ind_part | `VARCHAR(40)` | Nullable | 物料编号
Reference Table:Material |
| `location` | location | `VARCHAR(10)` | Nullable | 具体的物理库位编码 |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `due_date` | due_date | `DATE` | Nullable | 期望交付或就绪日期 |
| `supply` | supply | `VARCHAR(18)` | Nullable | 供给编号 |
| `supply_item` | supply_item | `DOUBLE` | Nullable | 供给行项目 |
| `supply_schedule_line` | supply_schedule_line | `DOUBLE` | Nullable | 供给计划行 |
| `request_constraint_start_date` | request_constraint_start_date | `DATE` | Nullable | 需求日期,如果是Make=LCS(Last ConstrainStartDate) |
| `available_date` | available_date | `DATE` | Nullable | 实际供应可用或可承诺交付日期 (ATP Date) |
| `request_part` | request_part | `VARCHAR(40)` | Nullable | 需求物料号 |
| `assigned_part` | assigned_part | `VARCHAR(40)` | Nullable | 供给物料号 |
| `dimension_grp` | dimension_grp | `VARCHAR(10)` | Nullable | 消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配 |
| `supply_type` | supply_type | `VARCHAR(10)` | Nullable | 供给来源 |
| `request_parent` | request_parent | `VARCHAR(40)` | Nullable | 父节点物料 |
| `ECS` | available_constraint_start_date | `DATE` | Nullable | 最早开工日期 |
| `part_ready_date` | part_ready_date | `DATE` | Nullable | 最早物料齐套日期 |
| `demand_source` | demand_source | `VARCHAR` | Nullable | reservation
forecast
sto
sales_order
allotment |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多对多供需 Pegging 双向钉结与优先级分配
* **因果流向**：`ipc_supply_assignment` 记录了实际独立/依赖需求与底层供应（OnHand 在手库存、SR 在途采购、PO 制造工单）之间的物理 Pegging 钉结关系。该表反映了最底层“供需对账”的流向，直接决定了 CTP 延迟原因归因分析（Late-delivery Gating Analysis）的准确性。
* **Pegging 算法编排**：
  1. 需求与供应排序：需求端按 Composite Priority 位权降序排列，供应端按可用日期（ATP Date）升序排列。
  2. 双滑动指针对撞分配：使用 FIFO 逻辑，将指针所指的需求与供应进行数量抵扣：
     $$ Pegged\_Qty = \min(Unmet\_Demand\_Qty, Available\_Supply\_Qty) $$
  3. 创建 Pegging 链路：如果供应早于需求交付，生成正常 Pegging；若供应落后于需求，生成延迟 Pegging 并计算延期天数。
  4. 二次调整：在 CTP 预占失败回滚时，引擎顺着该表定义的拓扑路径释放被锁定的供应量。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 引擎中，供需 Pegging 关系是基于扁平的双向无权有向图（Bipartite Graph）表示，在内存中连续存放以避免指针跳转：
```cpp
// 对应 ipc_supply_assignment 的 C++ 内存物理对齐结构体
struct SupplyAssignmentRecord {
    uint32_t demand_id;          // 需求 ID
    uint8_t demand_type;         // 需求类型 (1=SalesOrder, 2=Forecast, 3=Dependent)
    uint32_t supply_id;          // 供应 ID
    uint8_t supply_type;         // 供应类型 (1=OnHand, 2=SR, 3=PlannedOrder)
    uint32_t part_id;            // 物料 ID
    double pegged_qty;           // 钉结分配数量 (对应 qty)
    int pegging_day;             // 交付相对天数
};
```

###### 3. 边界与异常处理
* **在途供应取消级联处理**：若某张在途采购单（SR）被外部 ERP 系统标记为取消（Deleted），引擎会顺着该表中的 Pegging 链条回溯，直接将受影响的下游需求标记为“缺料延迟”，并触发 CTP 重排产以补充新计划订单（Planned Order）。