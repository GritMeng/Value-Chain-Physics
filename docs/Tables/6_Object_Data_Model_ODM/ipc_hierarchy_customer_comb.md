---
table_name: "ipc_hierarchy_customer_comb"
alias: "customer_comb_hierarchy"
module: "6_Object_Data_Model_ODM"
cpp_struct: "CustomerCombRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_hierarchy_customer_comb` (customer_comb_hierarchy)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `parent_region` | parent_region | `VARCHAR(40)` | Nullable | - |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `root` | root | `DATE` | Nullable | - |
| `level` | level | `VARCHAR(10)` | Nullable | 用于在分解计算中包括或排除聚合部件客户。 |
| `region` | region | `VARCHAR(10)` | Nullable | 地区编码 |
| `per_qty` | per_qty | `VARCHAR` | Nullable | - |
| `ratio` | ratio | `VARCHAR` | Nullable | 由历史数据可得 |
| `ratio_override` | ratio_override | `VARCHAR` | Nullable | 手工指定。当此值不为空时，以此数值为准 |
| `allocation_level` | allocation_level | `VARCHAR` | Nullable | - |
| `order_priority` | order_priority | `VARCHAR` | Nullable | - |
| `parent_product` | parent_product | `VARCHAR(40)` | Nullable | - |
| `parent_customer` | parent_customer | `VARCHAR` | Nullable | - |
| `customer` | customer | `VARCHAR` | Nullable | - |


##### 🔄 业务与报表集成设计 (BI Reporting & Metadata Specification)

###### 1. 业务说明与报表集成关系 (Reporting Rollups)
* **多维渠道组合报表**：`ipc_hierarchy_customer_comb` 提供了针对特殊销售渠道（如大KA、经销商、直销）的虚拟客户组合分类树。用于在 Consensus Forecast 审查会中展示特定渠道组的预测达成率，供销售副总裁调阅。
* **物理内存结构**：
```cpp
// 对应客户群组映射
struct CustomerCombRecord {
    uint32_t customer_id;
    uint32_t group_class_id; // 渠道组合分类逻辑编码
};
```