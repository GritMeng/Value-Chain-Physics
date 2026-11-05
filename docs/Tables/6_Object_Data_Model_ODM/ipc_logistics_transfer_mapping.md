---
table_name: "ipc_logistics_transfer_mapping"
alias: "transfer_mapping"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_logistics_transfer_mapping` (transfer_mapping)

> **业务说明**: 用于建立集团内转出的关系

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `transfer_order` | transfer_order | `VARCHAR(18)` | PK / NOT NULL | 唯一标识符号 |
| `lead_time` | lead_time | `DECIMAL(18,2)` | Nullable | 发出到接收的时间，时间单位为PlanningCalendar的时间单位 |
| `transfer_cost` | transfer_cost | `DECIMAL(18,2)` | Nullable | 转储成本 |
| `source_material` | source_material | `VARCHAR(40)` | Nullable | 源物料 |
| `to_part` | to_part | `VARCHAR(40)` | Nullable | 目标物料 |
| `souce` | souce | `VARCHAR(10)` | Nullable | 源 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：跨站点内协调拨的路径抉择
* **因果流向**：`ipc_logistics_transfer_mapping` 记录各厂区之间的供货关系（如 Site A 可从 Site B 补充物料 X）。
* **调拨算法编排**：
  在 IOP CTP 预占时，若当前站点的 ATP 库存和产能全部告罄，系统根据 `transfer_mapping` 中定义的 `transfer_cost` 与 `lead_time`，触发“跨厂区调拨回溯”。引擎在 $O(1)$ 时间内进行时间轴平移偏置，在 From Site 发起级联子需求；同时将 `transfer_cost` 计入此履约路径的总成本，评估是否超越其他替代工艺路线，作为多准则决策（MCDM）的路径优选基础。