---
table_name: "ipc_forecast_statistical"
alias: "statistical_forecast"
module: "4_IBP_Consolidated"
cpp_struct: "StatisticalForecastRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_statistical` (statistical_forecast)

> **业务说明**: statistical_forecast表将forecast统计函数计算的结果报告为未来日期的数量。这些计算基于statistical_forecastFit表中包含的统计模型参数和常量。如果在causal_factordetail和forecast_item_parameters_outlier表中报告了数量，则会在统计预测计算中考虑它们。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `prediction_parameters` | prediction_parameters | `VARCHAR` | PK / NOT NULL | - |
| `date` | date | `VARCHAR` | Nullable | - |
| `quantity` | quantity | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：基于平滑与 ARIMA 混合框架的统计预测
* **因果流向**：统计预测是整个 S&OP 需求流的输入底座。引擎读取 `ipc_his_demand_actual` 的历史发货与实际订单数据，自动剔除因果因素（Causal Factor）和异常离群值后，调用内置的统计算法生成 `ipc_forecast_statistical`。
* **算法模型编排**：
  1. 异常检测与清洗：使用 Z-score 法识别历史销量中的突发性离群值：
     $$ Z_t = \frac{Y_t - \mu}{\sigma} $$
     若 $|Z_t| > 3.0$，则使用中位数插值对该期数据进行置换清洗。
  2. 算法自动选型（Auto-Select）：计算历史需求的自相关系数（ACF）与偏自相关系数（PACF）。对具有明显周期性的 SKU 选择 Triple Exponential Smoothing (Holt-Winters 加法/乘法模型)，对非平稳趋势型 SKU 运行 ARIMA(p,d,q)。
  3. 平滑计算：以 Holt-Winters 加法模型为例，递推状态包括水平 $L_t$、趋势 $T_t$ 和季节因子 $I_t$：
     $$ L_t = \alpha(Y_t - I_{t-p}) + (1-\alpha)(L_{t-1} + T_{t-1}) $$
     $$ T_t = \beta(L_t - L_{t-1}) + (1-\beta)T_{t-1} $$
     $$ I_t = \gamma(Y_t - L_t) + (1-\gamma)I_{t-p} $$
  4. 拟合优度校验：计算 MAPE 与 R-squared，将拟合参数写入 `ipc_forecast_statistical_fit`，预测数量写入本表。

###### 2. 物理内存结构设计 (C++ DOD Layout)
统计预测模块直接跑在一维内存缓存的 `StatisticalForecastRecord` 数组中，便于在 SIMD 并行循环中进行快速累加和趋势平移：
```cpp
// 对应 ipc_forecast_statistical 表的内存物理结构体
struct StatisticalForecastRecord {
    uint32_t part_id;            // 物料 ID
    uint32_t customer_id;        // 客户 ID
    int day_bucket;              // 时序预测对应的计划天数 (对应 date)
    double qty;                  // 统计预测产生的值 (对应 quantity)
    double lower_bound_95;       // 95% 置信区间下限
    double upper_bound_95;       // 95% 置信区间上限
};
```

###### 3. 边界与异常处理
* **冷启动数据不足**：对于新建 SKU 或历史销量少于 2 个完整周期的物料，统计预测引擎会降级为简易的移动平均模型（Moving Average），避免参数拟合矩阵奇异报错。
* **异常趋势失控（Explosion）**：若预测趋势因子导致预测销量随时间无限发散，引擎将利用 `Part.max_sales_limit` 限制其绝对值，防止后续 MPS/MRP 生成非理性的巨量物料计划订单。