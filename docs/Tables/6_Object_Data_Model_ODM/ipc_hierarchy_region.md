---
table_name: "ipc_hierarchy_region"
alias: "region_hierarchy"
module: "6_Object_Data_Model_ODM"
cpp_struct: "RegionHierarchyNode"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_hierarchy_region` (region_hierarchy)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `region` | region | `VARCHAR(10)` | PK / NOT NULL | 地区编码 |
| `region_desc` | region_desc | `VARCHAR` | Nullable | 例如中东 |
| `parent` | parent | `VARCHAR(10)` | Nullable | - |
| `parent_desc` | parent_desc | `VARCHAR` | Nullable | - |
| `ratio` | ratio | `DECIMAL(18,2)` | Nullable | - |
| `hierarchy_type` | hierarchy_type | `VARCHAR(10)` | Nullable | - |


##### 🔄 业务与报表集成设计 (BI Reporting & Metadata Specification)

###### 1. 业务说明与报表集成关系 (Reporting Rollups)
* **地理维度汇总看板**：`ipc_hierarchy_region` 建立了物流网络及销售网点的地理维度树（如：华东 DC、华南 DC ➔ 中国区 ➔ 亚太区）。在进行全球物流调拨成本计算与控制塔（Control Tower）GIS 地图渲染时，引擎通过该表计算区域级别的在途库存和准时交付率。
* **物理内存结构**：
```cpp
// 对应地理区域树
struct RegionHierarchyNode {
    uint32_t region_id;          // 区域 ID (对应 region)
    uint32_t parent_region_id;   // 父区域 ID (对应 parent)
    uint8_t hierarchy_level;     // 级联层级
};
```