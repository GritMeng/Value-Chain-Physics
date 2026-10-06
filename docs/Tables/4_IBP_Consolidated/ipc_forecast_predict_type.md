---
table_name: "ipc_forecast_predict_type"
alias: "predict_type"
module: "4_IBP_Consolidated"
cpp_struct: "IpcForecastPredictTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_predict_type` (predict_type)

> **业务说明**: 该表由PredictionParameters表引用。它包含用于计算给定预测项目的统计预测的参数。例如，在计算统计预测时使用的统计模型和存储间隔在此表中标识。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `prediction_type` | prediction_type | `VARCHAR(10)` | PK / NOT NULL | - |
| `prediction_model` | prediction_model | `VARCHAR(1)` | Nullable | 预测模型:
AdditiveHoltWintersMethod
ARIMA
DoubleExponentialSmoothing
ExponentialSmoothing
ForecastImport
MovingAverage |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |
| `interval_calendar` | interval_calendar | `VARCHAR` | Nullable | Reference Calendar |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：统计预测模型分类配置表。指定在进行时序预测时，特定物料类别所采用的方法（ARIMA, Holt-Winters 等）。
* **计算逻辑编排**：
  1. 模型实例化：基于 prediction_model 实例化时序求解器（如 ARIMA(p,d,q)）；2. 季节性周期载入：读取 interval_calendar 确定季节长度；3. 平滑因子更新：输入历史数据，使用极大似然估计或网格搜索优化模型常数。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_forecast_predict_type 的 C++ DOD 物理对齐结构体
struct IpcForecastPredictTypeRecord {
    std::string prediction_type; // prediction_type 字符串 (-)
    std::string descriotion; // descriotion 字符串 (-)
    std::string interval_calendar; // interval_calendar 字符串 (Reference Calendar)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。