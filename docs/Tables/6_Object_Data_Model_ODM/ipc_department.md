---
table_name: "ipc_department"
alias: "department"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_department` (department)

> **业务说明**: 部门信息

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `department` | department | `VARCHAR(10)` | PK / NOT NULL | - |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |


##### 🔄 业务与报表集成设计 (BI Reporting & Metadata Specification)

###### 1. 业务说明与报表集成关系 (Reporting Rollups)
* **部门归属核算**：`ipc_department` 存储了企业的行政组织部门。在进行 IBP 计划的销售业绩考核和 S&OP 各大区经理采购审批额度授权时，引擎通过部门字段进行费用和权限的归集，生成部门维度的预算偏差表。