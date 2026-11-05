---
table_name: "ipc_location"
alias: "location"
module: "6_Object_Data_Model_ODM"
cpp_struct: "LocationRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_location` (location)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `location_id` | location_id | `VARCHAR(10)` | PK / NOT NULL | 仓库内位置的标识码(例如，库存室)。 |
| `storage_type` | storage_type | `VARCHAR(10)` | Nullable | 存储类型
Reference Table: StorageType |
| `whare_house` | whare_house | `VARCHAR(10)` | Nullable | Reference Table: Wharehouse |
| `address` | address | `VARCHAR` | Nullable | Location位置，街道 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |


##### 🔄 业务与报表集成设计 (BI Reporting & Metadata Specification)

###### 1. 业务说明与报表集成关系 (Reporting Rollups)
* **库位库龄看板与 WMS 对账**：`ipc_location` 记录了工厂或仓库内部的具体货架/库区位置。WMS（仓储管理系统）同步的在库库存在此表对账。在前台工作簿展示库位在库货量、拣货路线推荐时，此表提供基础物理拓扑信息。

###### 2. 物理内存结构设计 (C++ DOD Layout)
```cpp
// 对应货位元数据
struct LocationRecord {
    uint32_t location_id;       // 库位ID逻辑哈希 (对应 location)
    uint32_t site_id;           // 站点 ID
    bool is_nettable;           // 该库位库存是否可以参与 MRP 净需求扣减
};
```