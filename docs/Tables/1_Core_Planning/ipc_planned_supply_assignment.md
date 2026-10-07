---
table_name: "ipc_planned_supply_assignment"
alias: "planned_supply_assignment"
module: "1_Core_Planning"
cpp_struct: "PeggingRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 1_core_planning
---

# 🏷️ `ipc_planned_supply_assignment` (planned_supply_assignment)

> **业务说明**: 计划供应钉结分配表（Planned Pegging）。记录未锁定的计划订单（Planned Order）与独立需求之间的多对多钉结溯源关系，包含最早开工时间（ECS）与齐套时间。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `assigned_qty` | assigned_qty | `DECIMAL(18,2)` | PK / NOT NULL | 本次配额锁定的实际分配数量 |
| `assigned_parent_qty` | assigned_parent_qty | `DECIMAL(18,2)` | Nullable | 此supply分配的直接上层的数量 |
| `request_qty` | request_qty | `DECIMAL(18,2)` | Nullable | 原始请求的订货/预测数量 |
| `required_parent_qty` | required_parent_qty | `DECIMAL(18,2)` | Nullable | 此supply上层需求的数量 |
| `demand` | demand | `VARCHAR(18)` | 🔑 **PK / Required** | 需求编号 |
| `item` | item | `DOUBLE` | PK / NOT NULL | 需求行项目号 |
| `location` | location | `VARCHAR(10)` | Nullable | 具体的物理库位编码 |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `due_date` | due_date | `DATE` | Nullable | 期望交付或就绪日期 |
| `supply` | supply | `VARCHAR(18)` | Nullable | 供给编号 |
| `supply_item` | supply_item | `DOUBLE` | Nullable | 供给行项目 |
| `supply_schedule_line` | supply_schedule_line | `DOUBLE` | Nullable | 供给计划行 |
| `request_constraint_start_date` | request_constraint_start_date | `DATE` | Nullable | 需求日期,如果是Make=LCS(Last ConstrainStartDate) |
| `available_date` | available_date | `DATE` | Nullable | 实际供应可用或可承诺交付日期 (ATP Date) |
| `planned_order` | planned_order | `VARCHAR(18)` | Nullable | 计划订单号 |
| `Reservation` | document | `VARCHAR(18)` | Nullable | 预留单号 |
| `pla_site` | pla_site | `VARCHAR(8)` | Nullable | 需求site |
| `request_part` | request_part | `VARCHAR(40)` | Nullable | 需求物料号 |
| `assigned_part` | assigned_part | `VARCHAR(40)` | Nullable | 供给物料号 |
| `dimension_grp` | dimension_grp | `VARCHAR(10)` | Nullable | 消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配 |
| `supply_type` | supply_type | `VARCHAR(10)` | Nullable | 供给来源 |
| `request_parent` | request_parent | `VARCHAR(40)` | Nullable | 父节点物料 |
| `ECS` | available_constraint_start_date | `DATE` | Nullable | 最早开工日期 |
| `part_ready_date` | part_ready_date | `DATE` | Nullable | 最早物料齐套日期 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多对多供需双向 Pegging 钉结溯源
* **因果流向**：`ipc_planned_supply_assignment` 记录了需求（独立需求/预测）与供应（现存库存/在途订单/新计划订单）之间的动态 Pegging 链路，是控制塔展示“为什么订单延期”的底层数据依据。
* **Pegging 溯源编排**：
  1. MRP 展开时，引擎在消纳或生成供应后，同步将 Pegging 绑定记录追加至 `ipc_planned_supply_assignment` 表。
  2. 计算最早开工期（ECS）和物料齐套期（`part_ready_date`）：
     - 当子件缺料时，ECS 顺延，齐套期取所有子件中最晚到料时间。
     - 若发生替代，在 `assigned_part` 写入实际被分摊的替代料号，而 `request_part` 保持为原始需求物料。
  3. 异常归因：若发生产能瓶颈或断料，该表记录最底层的约束 ID，支撑控制塔前端直接从成品订单穿透定位到哪一个库区、哪台设备或哪家供应商发生了供应中断。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存计算期间，为了避免高并发写入的锁竞争，Pegging 记录在 DFS 回溯中临时存放在线程局部的事务栈中，直到整个履约树齐套成功后，一次性提交落库至 DuckDB 物理表中：

```cpp
// 关联 ipc_planned_supply_assignment 的 C++ 内存结构
struct PeggingRecord {
    uint32_t demand_id;  // 独立需求主键 (对应 demand)
    uint32_t part_id;    // 实际分配供给的物料 ID (对应 assigned_part)
    double qty;          // 分配绑定的物理数量 (对应 assigned_qty)
    int day;             // 分配可承诺就绪日期 (对应 available_date)
};

// 替代分配专属的 CTP 分配历史结构 (用于替代分摊审计)
struct AlternateAllocationRecord {
    uint32_t demand_id;    // 关联的需求订单 ID
    uint32_t main_part_id;  // 原始请求的主物料 ID
    uint32_t alt_part_id;   // 实际分摊替代件的物料 ID
    double allocated_qty;   // 实际分摊量
    int day;                // 交期天数
    int alt_class;          // 替代类别 (一类/二类/三类)
};
```