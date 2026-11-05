---
table_name: "ipc_logistics_delivery_route"
alias: "delivery_route"
module: "6_Object_Data_Model_ODM"
cpp_struct: "PartSiteTransitConfig"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_logistics_delivery_route` (delivery_route)

> **业务说明**: “配送路线”表用于对将成品货物运送到客户目的地的不同路线进行建模。例如，此处会指定订单从订单站点运送到客户收货地点所需的预计运输时间。该表中的许多字段在确定“独立需求”记录中的计划发货日期和客户收货日期时会发挥作用，同时这些字段也会用于确定这些记录中的可用发货日期和客户收货日期。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `delivery_route` | delivery_route | `VARCHAR(10)` | PK / NOT NULL | 此运输路线由的唯一字符串标识符。例如，这可能是一个Carrier和TransportationMode值的连接，或者其他一些惟一标识从站点发货到客户目的地之间的路由的字符串 |
| `carrier` | carrier | `VARCHAR(10)` | Nullable | 承运人沿着这条路线向客户运输货物.
Reference Table: Carrier |
| `transformation_mode` | transformation_mode | `VARCHAR(10)` | Nullable | 运输方式表示与此路线相关联的运输方式。
Land
Air
Sea
Train  
以此为主，Carrier为辅 |
| `shipment_calendar` | shipment_calendar | `VARCHAR(10)` | Nullable | 对日历的引用，该日历定义可从此站点发货的订单日期。
Reference Table: Calendar |
| `description` | description | `VARCHAR` | Nullable | 运输路线描述 |
| `destination` | destination | `VARCHAR(8)` | Nullable | 目的地
Reference Table: Customer |
| `ship_from` | ship_from | `VARCHAR(8)` | Nullable | 从哪个Site发出，跟Destination一起决定DeliceryRoute.
Reference Table: Site |
| `pick_calendar` | pick_calendar | `VARCHAR(10)` | Nullable | 拣配日历
Reference Table:Calendar |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多级配送网络时间偏置与交期承诺
* **因果流向**：`ipc_logistics_delivery_route` 定义了成品从出货仓运达客户目的地的时效。在 CTP 交期承诺计算中，该路线参数决定了客户要货期（Delivery Date）与发货期（Ship Date）之间的时间偏置：
  $$ Ship\_Date = Delivery\_Date - TransitLT - PickPackLT $$
  引擎沿交货路线的 `shipment_calendar`（发运日历）进行非工作日过滤，将发货期前推至最近的开班发货工作日。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存中，交货路线参数被扁平化编译为物料-站点配置 `PartSiteRecord` 中的 `transshipment_lead_time` 偏移标量，杜绝了排产时复杂的日历多表 Join 关联：

```cpp
// 对应 ipc_logistics_delivery_route 数据的 C++ 缓存优化标量
struct PartSiteTransitConfig {
    uint32_t delivery_route_id;
    int transit_lead_time;      // 运输提前期天数 (对应 TransitLT)
    int pick_pack_lead_time;    // 拣包提前期天数 (对应 PickPackLT)
};
```