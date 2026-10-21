---
table_name: "ipc_his_demand_category_rolling_weight"
alias: "his_demand_category_rolling_weight"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcHisDemandCategoryRollingWeightRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_his_demand_category_rolling_weight` (his_demand_category_rolling_weight)

> **业务说明**: 当使用滚动预测权重创建共识需求计划时，HisDemandCategoryRollingWeight表存储预测类别的预测权重记录。此表中的每条记录适用于一个预测类别，并指出该预测类别在特定水平上的权重。如果一个表头已经有了HistoricalDemandHeaderTimephasedAttributes，
HistoricalDemandHeaderRollingWeight或HistoricalDemandHeader记录，此表被忽略。
类型中未定义CalcForecastStartDate或CycleCalendar时，也会忽略该表
SOPConfiguration

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `category` | category | `VARCHAR` | PK / NOT NULL | - |
| `horizon` | horizon | `VARCHAR` | Nullable | - |
| `consensus_forecast_weight` | consensus_forecast_weight | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：历史分类滚动权重表。滚动计划周期内，定义不同历史需求类别在不同前置期（Lag）下的共识加权占比。
* **计算逻辑编排**：
  1. 滚动周期检索：根据当前 Lag 匹配对应的权重值；2. 共识折算：多历史源按权重融合成唯一的基准销量流，作为 S&OP 共识模型的基础历史依据。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_his_demand_category_rolling_weight 的 C++ DOD 物理对齐结构体
struct IpcHisDemandCategoryRollingWeightRecord {
    std::string category; // category 字符串 (-)
    std::string horizon; // horizon 字符串 (-)
    std::string consensus_forecast_weight; // consensus_forecast_weight 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。