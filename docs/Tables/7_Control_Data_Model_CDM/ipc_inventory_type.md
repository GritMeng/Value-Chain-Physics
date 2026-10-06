---
table_name: "ipc_inventory_type"
alias: "inventory_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "InventoryTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_inventory_type` (inventory_type)

> **业务说明**: 库存状态

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组 |
| `inventory_type` | inventory_type | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `operation_rule` | operation_rule | `VARCHAR(10)` | Nullable | 是否参与Netting
Y-参与
N-不参与 |
| `description` | description | `VARCHAR` | Nullable | 描述 |
| `specical_type` | specical_type | `VARCHAR(10)` | Nullable | 特殊类型
U - 非限制
K - VMI 
J - JIT
S - Sales Order Stock |


##### 🔄 业务与报表集成设计 (BI Reporting & Metadata Specification)

###### 1. 业务说明与报表集成关系 (Reporting Rollups)
* **库存状态分流与可用量对账**：`ipc_inventory_type` 定义了在库库存的物理状态类别（如：Nettable 正常可用、Quality Hold 质量冻结、Consignment 寄售等）。MRP 引擎在执行净需求扣减时，读取此表判断该状态库存是否属于可用资源（Nettable），并在财务报表中进行呆滞损失资产折算。

###### 2. 物理内存结构设计 (C++ DOD Layout)
```cpp
// 对应库存类型
struct InventoryTypeRecord {
    uint8_t inventory_type_id;   // 库存类型编码ID (对应 inventory_type)
    bool is_nettable_for_mrp;    // 是否为 MRP 可用库存
    bool is_allocated_for_ss;    // 是否已分配给安全库存
};
```