---
table_name: "ipc_hierarchy_customer"
alias: "customer_hierarchy"
module: "6_Object_Data_Model_ODM"
cpp_struct: "CustomerHierarchyNode"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_hierarchy_customer` (customer_hierarchy)

> **业务说明**: 客户层级

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `customer` | customer | `VARCHAR(10)` | Nullable | 唯一标识 |
| `description` | description | `VARCHAR` | Nullable | 客户组描述 |
| `address` | address | `VARCHAR` | Nullable | - |
| `parent_customer` | parent_customer | `VARCHAR(10)` | Nullable | - |
| `parent_description` | parent_description | `VARCHAR` | Nullable | - |


##### 🔄 业务与报表集成设计 (BI Reporting & Metadata Specification)

###### 1. 业务说明与报表集成关系 (Reporting Rollups)
* **因果流向与报表树**：`ipc_hierarchy_customer` 构成了销售需求与预测的客户维度聚合树。前端 BI 看板在生成 S&OP Consensus Forecast 合并报表、大客户销售利润分析、以及交付达成率（OTIF）仪表盘时，引擎通过此表将底层 `ipc_sales_order_line` 的明细订单自动向上卷算归集（Rollup）到母公司（如将 Walmart Online 和 Walmart Retail 聚合到 Walmart Corp）。
* **表间关系（Schema Joins）**：通过 `customer` 字段与交易表（如销售订单行、预测明细）执行 JOIN 关联，为前端分析工作簿提供汇总维度。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 引擎中，为了支持海量客户订单在内存中的快速聚合归集，客户树关系被哈希编译为扁平一维数组，以避免复杂的树状遍历：
```cpp
// 对应 ipc_hierarchy_customer 的内存 DOD 结构体
struct CustomerHierarchyNode {
    uint32_t customer_id;        // 客户 ID
    uint32_t parent_customer_id; // 父级大客户 ID
    uint8_t hierarchy_level;     // 树的层级深度
};
```