---
table_name: "ipc_constraint_load"
alias: "constraint_load"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_constraint_load` (constraint_load)

> **业务说明**: 约束负载

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `constraint` | constraint | `VARCHAR(10)` | PK / NOT NULL | 约束 |
| `con_part` | con_part | `VARCHAR` | PK / NOT NULL | - |
| `site` | site | `VARCHAR` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `supply_order` | supply_order | `VARCHAR(18)` | PK / NOT NULL | PlannedOrder，采购订单，生产订单。 |
| `planned_order` | planned_order | `VARCHAR(18)` | Nullable | 计划订单号 |
| `SupplySouce` | source | `VARCHAR(10)` | Nullable | PlannedOrder,ScheduleReceipt |
| `request_date` | request_date | `DATE` | Nullable | 需求日期 |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `load` | load | `DECIMAL(18,2)` | Nullable | 负载 |
| `supply_type` | supply_type | `VARCHAR(10)` | Nullable | 供给类型：MAKE, BUY,TRANSFER |
| `auto_or_manual` | auto_or_manual | `VARCHAR(10)` | Nullable | 当是PlannedOrder的时候看是手工输入还是系统自动生成 |
| `source_type` | source_type | `VARCHAR(10)` | Nullable | - |
| `demand_source` | demand_source | `VARCHAR` | Nullable | sr
planned_order |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：资源负荷追溯与对账
* **因果流向**：`ipc_constraint_load` 记录了每一个 PlannedOrder 或 ScheduledReceipt 在哪个瓶颈资源上、在什么日期占用了多少工时负荷。
* **数据流转**：CTP 预占引擎在 DFS 递归时，实时将产能占用写入内存中的 `ConstraintRecord.allocated_rates` 中。事务提交时，将明细扁平化输出到 `ipc_constraint_load`，供控制塔前端渲染“工作中心负荷负荷负荷”（Capacity Overload）图表，直观展现瓶颈负载。