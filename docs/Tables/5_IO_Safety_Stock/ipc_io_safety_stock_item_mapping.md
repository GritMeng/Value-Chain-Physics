---
table_name: "ipc_io_safety_stock_item_mapping"
alias: "safety_stock_item_mapping"
module: "5_IO_Safety_Stock"
cpp_struct: "SafetyStockItemMappingRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_io_safety_stock_item_mapping` (safety_stock_item_mapping)

> **业务说明**: SafetyStockItemMapping表用于将安全库存项目映射到一个或多个其他“Source”部件，这些“源”部件的历史需求、历史供应和/或未来需求数据应包含在确定该项目的推荐安全库存水平和重新订购点的计算中Item的
该表上的项目引用标识了应该计算安全库存水平的SafetyStockItem，而SourceMaterial引用标识了其历史供应、历史需求和历史预测数据被收集并用于生成这些安全库存水平的部分.
例如，映射到安全库存项目的源部件可能是在SafetyStockItem记录上定义的部件的不同版本。基于映射细节，然后将来自Source部分的特定数据与SafetyStockHistoricalDemand中为安全库存项生成的记录结合起来。
SafetyStockHistoricalSupply和SafetyStockItemFutureDemand表。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `id` | id | `VARCHAR` | PK / NOT NULL | 唯一标识 |
| `safety_stock_item` | safety_stock_item | `VARCHAR` | PK / NOT NULL | Reference |
| `include_future_demand` | include_future_demand | `VARCHAR` | Nullable | N - 不包括
Y - 源部件的未来需求数据从需求表中收集，并包含在报告的项目的桶总数中
SafetyStockItemFutureDemand表。 |
| `multiplier` | multiplier | `VARCHAR` | Nullable | 对源部件的参考，其历史需求、历史供应或未来需求数据应被收集并用于确定安全库存和安全库存项目的再订购点。
如果一个给定的安全库存项目需要多个类别的历史需求或供应数据，那么这可以作为项目引用同一部分。部分(以及从SafetyStockItem记录中选择的不同历史需求或供应类别) |
| `source_material` | source_material | `VARCHAR` | Nullable | Reference
对源部件的参考，其历史需求、历史供应或未来需求数据应被收集并用于确定安全库存和安全库存项目的再订购点。
如果一个给定的安全库存项目需要多个类别的历史需求或供应数据，那么这可以作为项目引用同一部分。部分(以及从SafetyStockItem记录中选择的不同历史需求或供应类别) |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：安全库存组配置规则分层映射
* **因果流向**：`ipc_io_safety_stock_item_mapping` 提供了在大批量物料管理中，批量参数映射的核心桥梁。它将一个通用的产品安全库存策略组（Product Safety Stock Group）与具体的物料 SKU 站点绑定，使求解器无需针对每个物料单独配置安全库存计算公式。
* **分层映射编排**：
  - 继承路径：当求解器计算某 SKU 的安全库存时，优先检索此映射表。若存在，继承该组定义的公式；若不存在，则使用系统全局默认策略。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 内存中，映射表在系统编译阶段被加载为紧凑的只读扁平哈希表或一维索引映射，以在多线程 MRP 循环中进行 O(1) 的策略指针分配：
```cpp
// 对应 ipc_io_safety_stock_item_mapping 的 C++ 内存结构体
struct SafetyStockItemMappingRecord {
    uint32_t product_group_id;        // 产品组逻辑ID
    uint32_t part_id;                 // 物料 ID (对应 part)
    uint32_t site_id;                 // 站点 ID (对应 site)
    uint32_t safety_stock_rule_id;    // 映射得到的安全库存计算规则ID
};
```

###### 3. 边界与异常处理
* **循环继承回溯判定**：若物料组 A 映射到 B，B 又反向映射到 A 形成死循环，预编译器在加载阶段会自动阻断，并降级为无安全库存策略，保障计划主干逻辑能够顺利执行。