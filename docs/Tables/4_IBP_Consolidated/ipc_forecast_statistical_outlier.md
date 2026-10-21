---
table_name: "ipc_forecast_statistical_outlier"
alias: "statistical_forecast_outlier"
module: "4_IBP_Consolidated"
cpp_struct: "StatisticalOutlierRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_statistical_outlier` (statistical_forecast_outlier)

> **业务说明**: StatisticalForecastOutlier表报告与为统计预测配置的项目的历史数据中的异常值相关的详细信息。对于这些预测项目，在生成统计预测时使用的每一段历史数据都会生成一条记录(由
PredictionParameters.HistoricalIntervalCount设置).

StatisticalForecastOutlier表中的每条记录表明给定的历史数据点是否代表数据集中的异常值，并包含其他有用的细节，例如该期间的需求数量以及应根据异常值进行调整的金额(如果有的话)。
请注意，此表仅为具有有效的预测项填充.
PredictionParameters.OutlierType参考。如果项目是PredictionParameters.OutlierType引用为“Null”，则该表中不会生成该项的记录。


| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `date` | date | `DATE` | Nullable | 此记录中报告的异常数据所对应的历史日期。这里报告的每个日期都属于项目的Type.IntervalsCalendar。例如，日期可能标记每周或每月周期的开始. |
| `existing_outlier_qty` | existing_outlier_qty | `DECIMAL(18,2)` | Nullable | 在ForecastItemParametersOutlier表中已经为项目和周期指定的离群量(如果有的话)。只有那些OperationRule被设置为“All”的PredictionParametersOutlier记录才会被报告并在该表中使用。 |
| `forecast` | forecast | `DECIMAL(18,2)` | Nullable | 表示一个值，该值可用于替换引用OutlierType记录的项的检测到的离群值，该记录具有AboveThreshold和/或中的“Forecast”设置
BelowThreshold字段 |
| `outlier` | outlier | `VARCHAR` | Nullable | 是否为异常值
Y
N |
| `prediction_parameters` | prediction_parameters | `VARCHAR` | Nullable | - |
| `low_threshold` | low_threshold | `DECIMAL(18,2)` | Nullable | - |
| `upper_thresholdmn` | upper_thresholdmn | `DECIMAL(18,2)` | Nullable | - |
| `adjustment` | adjustment | `DECIMAL(18,2)` | Nullable | 应根据异常值调整记录上的原始数量。这个建议的调整会反映在SuggestedQuantity字段中。
如果记录不表示离群值，则值为0(0)返回 |
| `qty` | qty | `DECIMAL(18,2)` | Nullable | 数量 (Quantity) |
| `suggested_qty` | suggested_qty | `VARCHAR` | Nullable | 在对任何检测到的异常值进行调整后，该项目在此期间的需求数量。
如果记录不表示异常值，则返回与Quantity字段相同的值。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：基于滑动 Z-score 的历史销售突发异值拦截与清洗
* **因果流向**：`ipc_forecast_statistical_outlier` 记录了历史数据中被识别出来的销量异常值（Outliers）。这些异常通常由客户突发的超大型一次性采购或供应链极端短缺引起。清洗这些离群值可以防止历史销量剧烈波动导致指数平滑法等模型的预测结果剧烈波动。
* **清洗算法编排**：
  1. 计算滑动窗口均值 $\mu_W$ 和标准差 $\sigma_W$。
  2. 计算当前实际销量 $Y_t$ 的偏差分数：
     $$ Z_t = \frac{Y_t - \mu_W}{\sigma_W} $$
  3. 拦截判定：若 $|Z_t| > Outlier\_Sigma$（通常设为 3.0），则将其判定为异常值，将 `is_outlier` 设为 True。
  4. 数量清洗：采用均值插值或中位数对销量进行向下修剪，并将修剪后的干净销量计入 `adjusted_demand_qty`，用于预测模型迭代。

###### 2. 物理内存结构设计 (C++ DOD Layout)
异常值明细在内存中以时序密集扁平数组存储，便于清洗算法进行快速滑动窗口运算：
```cpp
// 对应 ipc_forecast_statistical_outlier 的 C++ DOD 结构体
struct StatisticalOutlierRecord {
    uint32_t part_id;            // 物料 ID (对应 forecast_item)
    uint32_t customer_id;        // 客户 ID
    int day_bucket;              // 历史数据相对计划天数 (对应 date)
    double raw_demand_qty;       // 原始历史实际销量 (对应 quantity)
    double adjusted_demand_qty;  // 清洗后的有效销量 (对应调整量)
    bool is_outlier;             // 是否为离群值 (对应 outlier 标记)
};
```

###### 3. 边界与异常处理
* **连续极低销量异常判定**：对于低频销量（Lumpy Demand）物料，大多数天数出货量为 0，突发的一单会导致标准差极小、Z-score 极高，误判为 Outlier。引擎内部会进行销量频率校验，若零销量比例 $\ge 70\%$，则自动禁用 Z-score 算法，转而采用绝对上限门槛（Threshold Clamping）判定离群值。