---
table_name: "ipc_event_statistical_forecast_detail_adjustment"
alias: "event_statistical_forecast_detail_adjustment"
module: "4_IBP_Consolidated"
cpp_struct: "EventStatAdjustmentRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_event_statistical_forecast_detail_adjustment` (event_statistical_forecast_detail_adjustment)

> **业务说明**: 该表用于“事件管理”。它报告调整细节，包括受基于事件的调整影响的每个统计预测项目的数量调整。它类似于EventForecastDetailAdjustment表，它还包括关于对其他预测流的基于事件的调整的信息。
本表信息不用于计算共识预测。根据ForecastDetail表中统计预测流中的数据，对影响共识预测的统计预测项的调整将与EventForecastDetailAdjustment表和EventConsensusForecastDetail表中其他基于事件的调整一起报告。在某些情况下，这里报告的基于事件的调整细节之间可能存在差异

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `D` | date | `DATE` | Nullable | - |
| `eff_ad_qty` | eff_ad_qty | `VARCHAR` | Nullable | 在应用影响该预测项目的所有事件阶段之后，计算出的基于事件的数量调整 |
| `header` | header | `VARCHAR` | Nullable | Reference |
| `qty_adjustment` | qty_adjustment | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：历史大促离群值平滑调整系数校验
* **因果流向**：`ipc_event_statistical_forecast_detail_adjustment` 记录了针对历史大促产生的偏差调整系数。用于在模型计算拟合优度（MAPE）时，修正历史销量，防止大促引起的销量暴涨拉低了常规阶段的拟合得分。
* **物理内存结构**：
```cpp
// 对应较小颗粒度的调整系数
struct EventStatAdjustmentRecord {
    uint32_t part_id;
    int day_bucket;
    double historical_adjustment_multiplier;
};
```