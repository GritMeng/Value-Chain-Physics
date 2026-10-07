---
table_name: "ipc_event_statistical_forecast_detail"
alias: "event_statistical_forecast_detail"
module: "4_IBP_Consolidated"
cpp_struct: "EventStatisticalForecastDetailRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_event_statistical_forecast_detail` (event_statistical_forecast_detail)

> **业务说明**: 该表用于“事件管理”。它包含有关统计预测项的信息
受事件阶段的影响。它的结果与其他表中的数据一起用于计算
中列出的基于事件的统计预测调整
EventStatisticalForecastDetailAdjustmenttable

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `D` | date | `DATE` | Nullable | - |
| `header` | header | `VARCHAR` | Nullable | Reference |
| `item_parameters` | item_parameters | `VARCHAR` | PK / NOT NULL | Reference |
| `quantity` | quantity | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：基于历史大促系数的统计预测增量前置分配
* **因果流向**：`ipc_event_statistical_forecast_detail` 存储了根据历史同类型事件（如去年的国庆大促准时系数）拟合出的统计增量。系统利用该表将历史大促的“销量峰值”在前置期内进行平滑分配，防止预测模型误判为随机噪音。
* **分配算法**：
  1. 拟合历史大促提升因子 $Lift\_Multiplier$。
  2. 在新事件发生时，提取统计基准 $Baseline$，乘以上述提升因子，得出前置分配增量。

###### 2. 物理内存结构设计 (C++ DOD Layout)
```cpp
// 对应 ipc_event_statistical_forecast_detail 的 C++ 内存物理对齐结构
struct EventStatisticalForecastDetailRecord {
    uint32_t event_id;               // 关联事件 ID
    uint32_t part_id;                // 物料 ID
    int day_bucket;                  // 计划相对天数
    double baseline_forecast_qty;    // 常规统计预测基准量
    double event_lift_multiplier;    // 历史大促提升系数
    double final_adjusted_qty;       // 最终叠加后的调整量
};
```

###### 3. 边界与异常处理
* **历史样本不足平滑降级**：若该事件类型在历史数据中发生少于 2 次，引擎自动将 $Lift\_Multiplier$ 降级设为 1.0，仅保留手工录入值，以防过度预测。