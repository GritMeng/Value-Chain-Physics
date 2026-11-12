---
table_name: "ipc_event_type"
alias: "event_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_event_type` (event_type)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `type` | type | `VARCHAR` | PK / NOT NULL | 时间的分类 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |


##### 🔄 业务与报表集成设计 (BI Reporting & Metadata Specification)

###### 1. 业务说明与报表集成关系 (Reporting Rollups)
* **活动大促大类分类**：`ipc_event_type` 维护大促的性质类别（如 Promotion 价格大促、Holiday 节假日、NewProduct 新首发、Competitor 竞对动态）。用于生成促销 ROI 分析工作簿，对比不同活动类型带来的销售提升率（Lift Rate）和净利润贡献。