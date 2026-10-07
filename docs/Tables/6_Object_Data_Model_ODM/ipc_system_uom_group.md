---
table_name: "ipc_system_uom_group"
alias: "uom_grp"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_system_uom_group` (uom_grp)

> **业务说明**: UOMGroup表用于对单位类别进行分组。通常，同一类别中的单位可以用来表示该比例的数值。例如，一个类别可能是质量、体积或计数。属于某个类别的单位存储在UnitOfMeasure表中，包括该类别中单位之间的相对比率。例如:毫米、厘米、米和公里可能是类别“长度”的值。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `uom_grp` | uom_grp | `VARCHAR` | PK / NOT NULL | - |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |


##### 🔄 业务与报表集成设计 (BI Reporting & Metadata Specification)

###### 1. 业务说明与报表集成关系 (Reporting Rollups)
* **单位组核算**：`ipc_system_uom_group` 规定了计量单位的类别（如长度组、重量组、件数组），用于拦截前台工作簿中非法单位转换（如禁止将 KG 转换为厘米）。