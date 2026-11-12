---
table_name: "ipc_hierarchy_type"
alias: "hierarchy_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_hierarchy_type` (hierarchy_type)

> **业务说明**: 层级类型

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `hierarchy_type` | hierarchy_type | `VARCHAR(10)` | PK / NOT NULL | - |
| `ratio_rule` | ratio_rule | `VARCHAR(10)` | Nullable | Specifies how the ratio is used.Valid
values are:
Ignore—do not use the OptionRatio field in explosion
Fraction—value from 0 through 1 (1 means used in all
assemblies)
Percent—value from 0 through 100 (100 means used in
all assemblies)
 |


##### 🔄 业务与报表集成设计 (BI Reporting & Metadata Specification)

###### 1. 业务说明与报表集成关系 (Reporting Rollups)
* **维度类型切换器**：`ipc_hierarchy_type` 用于标识是哪种层级结构类型（如组织架构层级、地理位置层级）。BI 系统读取此表以控制报表下钻（Drill-down）的路径选择。