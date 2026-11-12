---
table_name: "ipc_source"
alias: "source"
module: "6_Object_Data_Model_ODM"
cpp_struct: "SourceRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_source` (source)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `source` | source | `VARCHAR(10)` | PK / NOT NULL | 唯一标识符 |
| `description` | description | `VARCHAR` | Nullable | Souce的描述 |
| `region` | region | `VARCHAR(10)` | Nullable |  Source的国家，地区 |
| `address` | address | `VARCHAR` | Nullable | 地址, 我们在Control Tower中可以显示Map供应网络，所以需要非常具体准确。 |
| `source_site` | source_site | `VARCHAR(8)` | Nullable | 负责采购的site, Reference Table: Site |
| `carrier` | carrier | `VARCHAR(10)` | Nullable |  承运商,Reference Table:Carrier |
| `contact_name` | contact_name | `VARCHAR(10)` | Nullable | 常用联系人姓名 |
| `email` | email | `VARCHAR(10)` | Nullable |  电邮 |
| `lt_units` | lt_units | `VARCHAR(10)` | Nullable | 提前期时间单位,Reference Table: calendar |
| `phone` | phone | `VARCHAR(10)` | Nullable | 电话号 |
| `transit_calendar` | transit_calendar | `VARCHAR(10)` | Nullable | 运输日历 Reference Table: calendar |
| `ship_calendar` | ship_calendar | `VARCHAR(10)` | Nullable | 发货日历 Reference Table: calendar |
| `transportation_model` | transportation_model | `VARCHAR(10)` | Nullable | 运输方式 Reference Table: TransformationModel |
| `source_calendar` | source_calendar | `VARCHAR(10)` | Nullable | 接收订单Calendar Reference Table: calendar |
| `pre_ship_lt` | pre_ship_lt | `DECIMAL(18,2)` | Nullable | 在供应物料使用该来源时，从供应方库存日期到发货日期之间允许的“源前置时间单位”数量。此字段用于留出时间让该来源收集库存并准备发货。小于零的值将被视为零。 |
| `transit_lt` | transit_lt | `DECIMAL(18,2)` | Nullable | 从采购站点将物资运输至目的地站点所需的时间。此字段表示“发货日期”与“卸货日期”之间的间隔时间，通常仅适用于转运来源。应以“运输日历”间隔来表示（或者如果“运输日历”引用为空，则使用“日常”日历）。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多站点网络流运输提前期平移与运费结算
* **因果流向**：`ipc_source` 定义了多级供应链网点之间物料流转的运输通道和提前期。当分销中心发生缺货、需要向中心制造工厂发出调拨需求（Stock Transfer Order）时，计划引擎读取该表以评估物流时效和物流分摊成本。
* **物流平移编排**：
  1. 时空平移（Lead Time Offset）：调拨单的发货日期（Ship Date）与到货日期（Due Date）之间，根据 `transit_lead_time_days` 执行时间轴向右平移：
     $$ Ship\_Day = Due\_Day - transit\_lead\_time\_days $$
  2. 运费计算：基于运输物料的件数和运费系数计算转储费用，并在 IBP 分类账中记录为物流持有成本。

###### 2. 物理内存结构设计 (C++ DOD Layout)
运输边在 C++ 引擎中表现为物流网络拓扑图的“边”（Edges），存储于有向网络图邻接表中：
```cpp
// 对应 ipc_source 的 C++ DOD 内存结构体
struct SourceRecord {
    uint32_t source_id;               // 运输路线 ID
    uint32_t part_id;                 // 物料 ID
    uint32_t origin_site_id;          // 起始站点 ID
    uint32_t dest_site_id;            // 目标目的站点 ID
    double transit_lead_time_days;    // 运输在途提前期
    double transfer_cost_per_unit;    // 每单位物料的转运成本
};
```

###### 3. 边界与异常处理
* **在途关口拥堵时效拉伸**：在发生口岸拥堵时，引擎读取运费调整系数，动态将 `transit_lead_time_days` 乘以拥堵拉伸乘数，防止由于实际到货延迟引起下游工厂停工待料。