---
table_name: "ipc_event_forecast_detail_adjustment"
alias: "event_forecast_detail_adjustment"
module: "4_IBP_Consolidated"
cpp_struct: "EventForecastDetailAdjustmentRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_event_forecast_detail_adjustment` (event_forecast_detail_adjustment)

> **业务说明**: 该表用于“事件管理”。它报告调整细节，包括受基于事件的调整影响的每个预测项目的任何数量或单价调整.
对于每个受影响的预测项目，此表中包含一个单独的记录
EventPhase.Calendar，并计算应用于该时间的所有基于事件的预测调整的结果。例如，如果给定日期桶中的预测项目受到使单价增加$10的事件阶段、使数量减少100的事件阶段和使数量增加50的事件阶段的影响，则AverageUnitPriceAdjustment字段中的值将为$10，而QuantityAdjustment字段中的值将为-50。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `average_unit_price` | average_unit_price | `DOUBLE` | Nullable | 有效单价的加权平均值ForecastDetail记录给定的Header和由Date指定的bucket。
如果在相应的时间段内没有ForecastDetail记录，则计算该值的方法与的ForecastDetail表上的有效单价
日期。 |
| `average_unit_price_adjustment` | average_unit_price_adjustment | `DOUBLE` | Nullable | 在应用影响该预测项目的所有事件阶段之后，计算出的基于事件的单位价格调整。 |
| `consensus_forecast` | consensus_forecast | `VARCHAR` | Nullable | 共识预测销售收入 (Qty * UnitPrice) |
| `D` | date | `DATE` | Nullable | - |
| `eff_qty` | eff_qty | `VARCHAR` | Nullable | 时间调整之前的数量 |
| `eff_ad_qty` | eff_ad_qty | `VARCHAR` | Nullable | 在应用影响该预测项目的所有事件阶段之后，计算出的基于事件的数量调整 |
| `original_qty` | original_qty | `VARCHAR` | Nullable | ForecastDetail的总和。数量以给定记录
头和由Date指定的桶。
如果在相应时间段内没有ForecastDetail记录，则该值为0 |
| `header` | header | `VARCHAR` | Nullable | Reference |
| `material` | material | `VARCHAR` | PK / NOT NULL | Reference |
| `qty_adjustment` | qty_adjustment | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多重营销事件需求增量级联叠加算法
* **因果流向**：`ipc_event_forecast_detail_adjustment` 记录了各个具体营销事件对预测细节的微调值。它是促销增量在分解到最底层物料客户层级后的物理表达，直接参与净需求拉动。
* **叠加算法编排**：
  - 时窗匹配：检索未来营销活动的生效区间。
  - 级联叠加：当多个事件发生时间重叠时，引擎依据事件的叠加优先级，以加法或乘法系数级联作用于基本预测，生成该表的调整量记录。

###### 2. 物理内存结构设计 (C++ DOD Layout)
促销调整明细在内存中以密集排序向量存储，优化了双指针合并的时间复杂度：
```cpp
// 对应 ipc_event_forecast_detail_adjustment 的内存物理结构
struct EventForecastDetailAdjustmentRecord {
    uint32_t event_id;           // 事件 ID
    uint32_t part_id;            // 物料 ID
    uint32_t customer_id;        // 客户 ID
    int day_bucket;              // 相对计划天数 (对应 date)
    double qty_adjustment;       // 数量调整绝对值 (对应 qty_adjustment)
    double price_adjustment_pct;  // 价格调整百分比
};
```

###### 3. 边界与异常处理
* **负数需求异常截断**：当营销退货活动（Negative Event）调整量大于基准预测，导致叠加后需求变为负数时，系统强行将其截断为 0，防止 MRP 阶段算出负数采购单。