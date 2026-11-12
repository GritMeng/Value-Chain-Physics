---
table_name: "ipc_forecast_prediction_actuals"
alias: "prediction_actuals"
module: "4_IBP_Consolidated"
cpp_struct: "IpcForecastPredictionActualsRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_prediction_actuals` (prediction_actuals)

> **业务说明**: PredictionActual表将一组历史实际需求与为统计预测计算的参数相匹配，从而允许您度量预测方法与它所基于的点的匹配程度。换句话说，PredictionActual表允许您度量模型与实际数据的拟合程度，而不必在预测中发现错误。
PredictionActual表中的计算基于StatisticalForecastFit表产生的统计模型参数和常数.

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `actual_qty` | actual_qty | `VARCHAR` | Nullable | 实际发货数量 |
| `forecast_qty` | forecast_qty | `VARCHAR` | Nullable | 预测数量 |
| `date` | date | `VARCHAR` | Nullable | - |
| `prediction_patameters` | prediction_patameters | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：预测拟合对比表。用于在训练阶段评估模型对历史实际销量的拟合质量，不引入远期误差误差判定。
* **计算逻辑编排**：
  1. 拟合度指标核算：对比 actual_qty 与 forecast_qty，计算均方根误差（RMSE）与平均绝对百分比误差（MAPE）；2. 离群值标记：识别残差超过三倍标准差的异常历史数据，标记为 Outlier，反馈至参数配置表。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_forecast_prediction_actuals 的 C++ DOD 物理对齐结构体
struct IpcForecastPredictionActualsRecord {
    std::string actual_qty; // actual_qty 字符串 (实际发货数量)
    std::string forecast_qty; // forecast_qty 字符串 (预测数量)
    std::string date; // date 字符串 (-)
    std::string prediction_patameters; // prediction_patameters 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。