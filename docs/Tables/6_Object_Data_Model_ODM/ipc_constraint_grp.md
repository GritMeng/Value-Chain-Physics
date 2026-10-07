---
table_name: "ipc_constraint_grp"
alias: "constraint_grp"
module: "6_Object_Data_Model_ODM"
cpp_struct: "ConstraintGroupRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_constraint_grp` (constraint_grp)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `constraint_group` | constraint_group | `VARCHAR(10)` | PK / NOT NULL | 唯一标识符 |
| `description` | description | `VARCHAR` | Nullable | 描述 |
| `constraint` | constraint | `VARCHAR(10)` | PK / NOT NULL | - |
| `priority` | priority | `DECIMAL(18,2)` | Nullable | 优先级，数值越小越优先 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：约束资源组划分与联合负荷对账
* **因果流向**：`ipc_constraint_grp` 定义了约束组属性，将多个相似功能的设备（如：SMT 装贴线 A、B、C）划分为一个组，在 S&OP 阶段进行宏观产能对账，并在 IOP 阶段支持跨设备的负载动态转移。
* **物理内存结构**：
```cpp
// 对应 ipc_constraint_grp 的内存结构
struct ConstraintGroupRecord {
    uint32_t constraint_group_id; // 约束组 ID (对应 constraint_grp)
    double aggregate_capacity;    // 组内累计产能上限
    bool allow_cross_routing;     // 是否允许组内设备自动分流
};
```