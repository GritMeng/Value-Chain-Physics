---
table_name: "ipc_forecast_actual_parameters"
alias: "actual_parameters"
module: "4_IBP_Consolidated"
cpp_struct: "ForecastActualParametersRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_actual_parameters` (actual_parameters)

> **业务说明**: 跟Actual相关的原始数据和调整数据.
包含预测项目的桶式调整历史记录，以及使用PredictParametersMap表指定的任何其他预测项目。调整后的预测项目的历史记录是预测项目的历史实际需求和因果因素的总和，由中定义的日历存储ForecastItemParameters.Type.IntervalsCalendar。此表反映了
ForecastItemParametersOutlier表.

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `date` | date | `DATE` | Nullable | Calendar Interval |
| `prediction_parameters` | prediction_parameters | `VARCHAR` | PK / NOT NULL | - |
| `outlier_qty` | outlier_qty | `VARCHAR` | Nullable | 包括了FromItem的数量汇总 |
| `qty` | qty | `VARCHAR` | Nullable | 数量 (Quantity) |
| `self_outlier_qty` | self_outlier_qty | `VARCHAR` | Nullable | 不包括FromItem的Causal数量 |
| `self_qty` | self_qty | `VARCHAR` | Nullable | 考虑Causal,不包括FromItem |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：历史实际销量参量化配置与预测冷启动窗口
* **因果流向**：`ipc_forecast_actual_parameters` 存储了配置历史销量如何参与预测模型的参数。它规定了在多大历史滑动区间内计算销量的均值和变异度，直接影响异常离群值判定的边界值。
* **参数应用编排**：
  - 提取 `historical_averaging_buckets` 作为时间周期滑动天数，动态计算滑动均值 $\mu$ 与方差 $\sigma^2$。

###### 2. 物理内存结构设计 (C++ DOD Layout)
```cpp
// 对应 ipc_forecast_actual_parameters 的 C++ DOD 物理数据结构
struct ForecastActualParametersRecord {
    uint32_t actual_parameters_id;      // 参数集主键 ID
    int historical_averaging_buckets;   // 历史销量统计滑动天数
    bool treat_outliers_as_zero;        // 是否将识别出的异常值设为 0 (否则设为中位数)
};
```

###### 3. 边界与异常处理
* **历史天数超限自收缩**：若设置的 `historical_averaging_buckets` 超过了数据库中实际存在的历史最长记录，系统会自动收缩时窗至最大可用记录长度，不予报错。