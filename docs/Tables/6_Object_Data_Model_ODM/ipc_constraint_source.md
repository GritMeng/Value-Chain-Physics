---
table_name: "ipc_constraint_source"
alias: "constraint_source"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_constraint_source` (constraint_source)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `description` | description | `VARCHAR` | Nullable | Souce约束的描述 |
| `constraint` | constraint | `VARCHAR(10)` | PK / NOT NULL | Reference Table: Constrain |
| `factor` | factor | `DECIMAL(18,2)` | Nullable | 每单位的需求消耗的Constrain数量，单位用PartSource中的单位 |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | 生效日期 |
| `fixed_factor` | fixed_factor | `DECIMAL(18,2)` | Nullable | 每笔Supply固定消耗Constrain的数量。 |
| `part` | part | `INTEGER` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `site` | site | `VARCHAR` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `source` | source | `VARCHAR` | PK / NOT NULL | - |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：物料-资源消耗系数匹配
* **因果流向**：`ipc_constraint_source` 定义了特定物料在特定站点下的设备消耗配额（单位消耗率 `factor` 及换型固定消耗 `fixed_factor`）。
* **排程编排**：工序排定开工后，系统依据此表的 `factor` 将工单数量折算为工时负荷，并在开工第一天额外扣减 `fixed_factor` 作为换型切换开销，直接挂载到 `ipc_resource_capacity` 的水位扣减上。