---
table_name: "ipc_forecast_statistical_fit"
alias: "statistical_forecast_fit"
module: "4_IBP_Consolidated"
cpp_struct: "StatisticalFitRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_statistical_fit` (statistical_forecast_fit)

> **业务说明**: 
StatisticalForecastFit表确定用于计算统计预测的统计模型参数，并计算统计模型的各种统计数据特征和误差度量。表中存储的数据将用于生成PredictActual表。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `prediction_mole` | prediction_mole | `VARCHAR` | Nullable | 预测模型:
AdditiveHoltWintersMethod
ARIMA
DoubleExponentialSmoothing
ExponentialSmoothing
ForecastImport
MovingAverage |
| `forecast_item` | forecast_item | `VARCHAR` | Nullable | - |
| `prediction_parameters` | prediction_parameters | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：预测模型拟合误差（MAPE/MAD）核算与最优模型自选择
* **因果流向**：`ipc_forecast_statistical_fit` 存储了统计预测模型的拟合参数和误差度量指标（MAPE、MAD、R-Square）。它是预测引擎执行“自动算法选型（Tournament Forecasting）”的评估指标库，指导系统自动选取误差最小的模型作为该 SKU 的主力预测算法。
* **拟合计算编排**：
  1. 误差计算：以 MAPE（平均绝对百分比误差）为例，对比历史预测值 $F_t$ 与历史实际发货值 $A_t$：
     $$ MAPE = \frac{100\%}{n} \sum_{t=1}^n \left| \frac{A_t - F_t}{A_t} \right| $$
  2. 选型淘汰赛（Tournament）：对同一物料并行运行移动平均、指数平滑、Holt-Winters 及 ARIMA 算法，计算拟合值，并将 MAPE 最小的算法类型更新至本表的 `prediction_mole` 中，作为最终的预测生成模型。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 内存中，拟合结果被紧凑地存放在 `StatisticalFitRecord` 结构中，用于支持前台预测看板的高速指标分析：
```cpp
// 对应 ipc_forecast_statistical_fit 的 C++ 内存物理对齐结构体
struct StatisticalFitRecord {
    uint32_t part_id;               // 物料 ID (对应 forecast_item)
    uint32_t customer_id;           // 客户 ID
    double mape_score;              // MAPE 拟合误差
    double mad_score;               // MAD 拟合误差
    double r_squared;               // R-squared 拟合优度 (0.0 - 1.0)
    uint8_t selected_model_type;    // 最终选用的预测算法模型枚举 (对应 prediction_mole)
};
```

###### 3. 边界与异常处理
* **实际发货量为零引起的除零异常（Zero Actuals）**：若某历史时段实际销量 $A_t = 0$，常规 MAPE 公式分母为零会产生 NaN 错误。引擎计算时会自动使用 MAD（平均绝对偏差）作为主要考核指标，或对分母加入微小的校正因子 $\epsilon = 0.001$，确保公式能够平稳运行。