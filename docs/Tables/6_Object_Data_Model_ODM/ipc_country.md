---
table_name: "ipc_country"
alias: "country"
module: "6_Object_Data_Model_ODM"
cpp_struct: "CountryRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_country` (country)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `Country` | country_id | `VARCHAR(10)` | PK / NOT NULL | 国家编码 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：全球供应链关税核算与跨国物流延迟补偿
* **因果流向**：`ipc_country` 存储了全球站点的国家主数据。在跨国配送路径上，物流引擎不仅需要计算地理距离，还必须通过该表查找目的国与起运国之间的基本关税税率，并为物流转移订单（STO）加上基准清关提前期（`base_customs_lead_days`）。
* **时效计算编排**：
  - 读取物流路线的目的国 ID。
  - 获取 `base_customs_lead_days`。
  - 将清关时延累加到发货提前期中，进行时空平移计算。

###### 2. 物理内存结构设计 (C++ DOD Layout)
```cpp
// 对应 ipc_country 表的 C++ DOD 物理数据结构
struct CountryRecord {
    uint16_t country_id;             // 国家 ID (对应 Country)
    double default_tariff_rate;      // 通用关税税率
    int base_customs_lead_days;      // 口岸基准清关滞留天数
};
```