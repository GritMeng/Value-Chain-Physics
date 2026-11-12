---
table_name: "ipc_his_demand_category_type"
alias: "his_demand_category_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcHisDemandCategoryTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_his_demand_category_type` (his_demand_category_type)

> **业务说明**: 用来指定HisDemandCategory的数据如何处理.

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `type` | type | `VARCHAR(1)` | PK / NOT NULL | 类别类型. |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 类别描述 |
| `operation_rule` | operation_rule | `VARCHAR(0)` | Nullable | Actual - 类别用于存储历史需求实际情况。这一类的值可用于计算统计预测。被共识预测计算忽略。
Forecast - 类别用于存储预测需求流或预测调整流。此类别的值可用于计算共识预测(基于指定的
ConsensusForecastWeight值)。
注意，在其他历史类别类型中指定的调整和覆盖值随后可用于修改或替换计算出的一致预测。
Target - 类别用于定义衡量S&OP年度计划的目标度量值。被共识预测计算忽略
ForecastOverride - 用于指定覆盖计算一致预测的值
(权重被忽略)。注意，这种类型的覆盖可以通过重新平衡调整或被
RebalancingForecastOverride。
None - 不参与共识预测
RebalancingAdjustment  - 用于增加或减少
ConsensusForecast。计算数量或
在需求和供应平衡阶段(忽略权重)期间，ForecastOverride值(如果使用)。
请注意，如果RebalancingForecastOverride被指定, 则此调整被忽略。
RebalancingOverride - 用于指定值，以在需求和供应平衡阶段覆盖计算的共识预测
(权重被忽略)。
注意，这种类型的重写优先于
ForecastOverride以及任何应用的RebalancingAdjustment值 |
| `disaggregation_rule` | disaggregation_rule | `VARCHAR(1)` | Nullable | 确定应该使用历史实际情况还是一致预测来计算此类型类别的预测分解率的值。
Actual - 历史实际数据来计算这类预测类别的分解率。
StatisticalForecast - 对于适用部分客户的一致预测，应用于计算这类预测类别的分解率。 |
| `unit_type` | unit_type | `VARCHAR(10)` | Nullable | 当Type = 'Target'时.
Qty - ForecastDetail和HistoricalDemandSeriesDetail Quantity字段中的值用作历史需求和预测的目标。
Value - ForecastDetail和HistoricalDemandSeriesDetail表的Value字段中的值用作历史需求和预测的目标。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：历史需求分类类型控制表。指定每一类历史需求属于 Actual 实际值、Forecast 预测值还是 Target 目标值。
* **计算逻辑编排**：
  1. 分流逻辑：定义 disaggregation_rule（历史实际还是统计预测）指导共识计划分解；2. 单位控制：若是 Target，控制数值字段是 Quantity（件数）还是 Value（金额）。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_his_demand_category_type 的 C++ DOD 物理对齐结构体
struct IpcHisDemandCategoryTypeRecord {
    std::string type; // type 字符串 (类别类型.)
    std::string descriotion; // descriotion 字符串 (类别描述)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。