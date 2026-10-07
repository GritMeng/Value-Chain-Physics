---
table_name: "ipc_event_consensus_forecast_detail"
alias: "event_consensus_forecast_detail"
module: "4_IBP_Consolidated"
cpp_struct: "EventConsensusForecastDetailRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_event_consensus_forecast_detail` (event_consensus_forecast_detail)

> **业务说明**: EventConsensusForecastDetail表用于事件管理。它保存有关应用了基于事件的预测调整的预测项目的信息，这些项目也属于用于生成一致预测的预测类别。的记录EventForecastDetailAdjustment表用作本表中记录的来源。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `average_unit_price` | average_unit_price | `DOUBLE` | Nullable | 有效单价的加权平均值
ForecastDetail记录指定日期桶的给定Header。
属性中的对应字段
EventForecastDetailAdjustment表 |
| `average_unit_price_adjustment` | average_unit_price_adjustment | `DOUBLE` | Nullable | 在应用影响该预测项目的所有事件阶段之后，计算出的基于事件的单位价格调整。 |
| `consensus_forecast` | consensus_forecast | `VARCHAR` | Nullable | 共识预测销售收入 (Qty * UnitPrice) |
| `D` | date | `DATE` | Nullable | - |
| `eff_qty` | eff_qty | `VARCHAR` | Nullable | 时间调整之前的数量 |
| `eff_ad_qty` | eff_ad_qty | `VARCHAR` | Nullable | 在应用影响该预测项目的所有事件阶段之后，计算出的基于事件的数量调整 |
| `event_forecast_detail_ad` | event_forecast_detail_ad | `VARCHAR` | Nullable | Reference |
| `header` | header | `VARCHAR` | Nullable | Reference |
| `material` | material | `VARCHAR` | PK / NOT NULL | Reference |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：促销事件共识预测合并与财务结账
* **因果流向**：`ipc_event_consensus_forecast_detail` 用于在 S&OP 活动管理中，将前端大促（Event）产生的临时增量或定价变动，合并并对账至最终的共识预测中。这决定了活动期间预估营业额（Revenue）的计算。
* **对账算法编排**：
  1. 基准共识获取：读取常规共识预测量 $Qty_{consensus}$。
  2. 事件增量叠加：累加所有在该日期生效的活动调整量 $Qty_{adjust}$ 和单价变动 $Price_{adjust}$：
     $$ Qty_{final}(t) = Qty_{consensus}(t) + \sum_{e \in Events} Qty\_Adjustment_e(t) $$
  3. 财务核算：折算最终的共识销售收入，写入 `ipc_financial_ledger`，供决策者评估促销 ROI。

###### 2. 物理内存结构设计 (C++ DOD Layout)
大促共识明细在内存中以时序扁平 SoA 数组分布，以便进行向量化累加：
```cpp
// 对应 ipc_event_consensus_forecast_detail 的 C++ DOD 结构体
struct EventConsensusForecastDetailRecord {
    uint32_t event_id;           // 促销事件 ID (对应 event_forecast_detail_ad)
    uint32_t part_id;            // 物料 ID (对应 material)
    int day_bucket;              // 计划相对天数 (对应 date)
    double baseline_qty;         // 基础共识量 (对应 eff_qty)
    double adjust_qty;           // 事件调整量 (对应 eff_ad_qty)
    double adjusted_unit_price;  // 调整后单价 (对应 average_unit_price_adjustment)
};
```

###### 3. 边界与异常处理
* **极端价格折扣拦截**：如果促销大促折扣导致折后有效单价低于标准成本的 50%，引擎会自动拦截并发出“毛利过低爆红”报警，但不会中断计算，仍以该折扣价折算营收。