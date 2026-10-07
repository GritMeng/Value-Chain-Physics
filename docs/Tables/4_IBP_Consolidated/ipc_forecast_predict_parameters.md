---
table_name: "ipc_forecast_predict_parameters"
alias: "predict_parameters"
module: "4_IBP_Consolidated"
cpp_struct: "ForecastPredictParametersRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_predict_parameters` (predict_parameters)

> **业务说明**: 此表包含用于生成给定预测项目的统计预测或用于生成给定安全库存项目的安全库存建议的输入参数。
请注意，每个预测项目可以生成几个统计预测，但是每个统计预测必须与不同的预测类别相关联。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `actual_category` | actual_category | `VARCHAR(1)` | Nullable | 关联HisDemandCategory的Actual, 用来产生Outlier |
| `constant` | constant | `VARCHAR(1)` | Nullable | 指示是否包含一个常数值(也称为Y截距)在ARIMA或
ARIMAX预测计算。取值包括:
Use - 在计算中使用常数。
Ignore—对于ARIMA，该常数不用于计算。对于ARIMAX, Ignore强制常数项为零 |
| `confidence` | confidence | `DECIMAL(18,2)` | Nullable | 一种置信水平，用于计算使用下列统计预测模型之一生成的统计预测的预测区间:
ARIMA
ARIMAX
Double exponential smoothing
Exponential smoothing
Holt-Winters (multiplicative and additive)
Linear
Multiple Linear Regression
Rforecast
Step-wise ARIMA

应该提供0.5到0.9999之间的值。任何>= 1的值都被视为0.9999来确定
StatisticalForecast.PredictionIntervalLower和
StatisticalForecast.PredictionIntervalUpper。任何<= 0的值表示忽略该字段。

注意:对于指定了非空引用的项，此字段中的值总是被忽略
predicastprofile字段，或者在BaseQuantity或
ScalingFactor字段。 |
| `ar` | ar | `INTEGER` | Nullable | p, 多少个单位计算. |
| `forecast_category` | forecast_category | `VARCHAR` | PK / NOT NULL | 类别为预测 |
| `predict_interval_counts` | predict_interval_counts | `DECIMAL(18,2)` | Nullable | 未来多少期产生预测 |
| `his_interval_counts` | his_interval_counts | `DECIMAL(18,2)` | Nullable | 取多少期的历史预测用于产生预测 |
| `forecast_item` | forecast_item | `VARCHAR` | PK / NOT NULL | reference ForecastItem |
| `ma` | ma | `INTEGER` | Nullable | q,多少个时间单位 |
| `prediction_type` | prediction_type | `VARCHAR(10)` | Nullable | Reference PredictionType |
| `outlier_moving
average_window` | outlier_moving
average_window | `INTEGER` | Nullable | 用于计算离群值检测的移动平均线的历史间隔数。
仅适用于OutlierType.DataRule设置为“MovingAverageError”，
它使用历史数据点和计算的移动平均线之间的差异来确定何时确定哪些点是异常值。 |
| `outlier_smoothing
after_interval_count` | outlier_smoothing
after_interval_count | `INTEGER` | Nullable | 平滑需求异常值时的安全性
库存项目，这表示默认的数量
类型。日历周期向前跨越
用来传播价值观。这发生在任何
向后传播。 |
| `outlier_smoothing
before_interval_count` | outlier_smoothing
before_interval_count | `INTEGER` | Nullable | 平滑安全库存项目的需求异常值时，这表示的默认数量
类型。interval日历周期向后扩展值。这发生在任何远期价差之后。 |
| `outlier_threshold` | outlier_threshold | `INTEGER` | Nullable | 为在生成统计预测时使用的历史数据中检测异常值设置阈值。取值必须大于0。
此值用于确定高于该值的点被视为离群值
(上阈值)和低于该值的点被视为异常值(下阈值)。
此字段中值的实际解释受OutlierType的影响。DetectionRule设置如下:
1. IglewiczHoaglinMethod - 修改后的Zscore。的z得分值应基于正态分布表。z分数小于2.33表示该数据点不是离群值的概率为99%。下表列出了概率及其对应的z分数:
% Prob Z-score
90.0 1.28
95.0 1.64
96.0 1.75
97.0 1.88
98.0 2.05
99.0 2.33
99.5 2.58
99.6 2.65
99.7 2.75
99.8 2.88
99.9 3.09
2. StandardDeviation - 离均值的标准差数。例如，3表示距离平均值超过三个标准差的点被认为是离群值.
3. Winsorizing - 数据中的一个百分位数。应该指定0.01到0.49之间的值。例如，0.05表示低于第5个百分位数或高于95%的百分位数被认为是异常值
 |
| `outlier_type` | outlier_type | `VARCHAR(10)` | Nullable | 异常值类型 |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | 计算该项目的预报的第一个日期。早于此日期的值被认为是零。
如果此值为Undefined，则使用Past |
| `eff_end_date` | eff_end_date | `DATE` | Nullable | 最后一次计算该项目的预报日期。迟于此日期的值被视为零。
如果该值为Undefined，则使用Future. |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：统计预测引擎参数管理与模型训练配置
* **因果流向**：`ipc_forecast_predict_parameters` 存储了驱动预测模型运行的超参数，包括三次指数平滑和 ARIMA 的各项平滑系数（$lpha, eta, \gamma$）、季节性周期窗口和历史训练跨度。它决定了统计预测引擎在生成未来基准销量时的响应灵敏度。
* **平滑系数计算编排**：
  1. 水平平滑常数 $lpha$：控制近期历史需求的反应权重，值越高，预测对最新市场变动越敏感。
  2. 趋势平滑常数 $eta$：控制趋势变化的惯性权重，用于避免由于突发促销导致趋势预测过度发散。
  3. 季节平滑常数 $\gamma$：用于调整历史季节因子在未来周期内的衰减。
  4. 拟合算法会在运行前执行网格搜索（Grid Search），最小化均方根误差（RMSE），自动更新并保存参数至该表。

###### 2. 物理内存结构设计 (C++ DOD Layout)
模型参数在物理内存中以紧凑的数据对齐格式存储，直接由多线程预测求解器调用：
```cpp
// 对应 ipc_forecast_predict_parameters 的内存物理结构
struct ForecastPredictParametersRecord {
    uint32_t parameters_id;            // 参数集ID (对应 prediction_parameters)
    double alpha_level_smoothing;      // 水平平滑系数 (对应 alpha)
    double beta_trend_smoothing;       // 趋势平滑系数 (对应 beta)
    double gamma_seasonal_smoothing;   // 季节平滑系数 (对应 gamma)
    int seasonal_intervals;            // 季节周期跨度 (对应 intervals)
    int history_window_days;           // 历史数据检索天数
    double outlier_sigma_threshold;    // 离群值异常检测 Z-score 阈值
};
```

###### 3. 边界与异常处理
* **非法系数越界拦截**：若网格搜索或人工误操作输入了不在 $[0.0, 1.0]$ 区间内的平滑系数值，引擎在初始化编译时会强制将其截断至边界，例如将 $1.2$ 截断为 $1.0$，并输出配置警告日志。