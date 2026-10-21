---
table_name: "ipc_his_demand_header_rolling_weight"
alias: "his_demand_header_rolling_weight"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcHisDemandHeaderRollingWeightRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_his_demand_header_rolling_weight` (his_demand_header_rolling_weight)

> **业务说明**: 当使用滚动预测权重来创建共识需求计划时，HisDemandHeaderRollingWeight表存储应用于水平的特定持续时间的标头的权重。如果报头中已有HisDemandHeaderTimephasedAttributes记录，则忽略该表。类型中未定义CalcForecastStartDate或CycleCalendar时，也会忽略该表.
SOPConfiguration表中的ForecastStartOffset字段会影响滚动权重汇总工作表(S&OP需求计划比率工作簿)中剩余水平期的计算方式。如果该值>0，则剩余周期从第0个月开始(然后在所有其他定义的滚动地平线结束后继续)

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `header` | header | `VARCHAR` | PK / NOT NULL | - |
| `horizon` | horizon | `VARCHAR` | PK / NOT NULL | - |
| `consensus_forecast_weight` | consensus_forecast_weight | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：大客户专属滚动权重表。针对特定 SKU 或客户头，在滚动周期内覆盖通用的滚动加权配置。
* **计算逻辑编排**：
  1. 专属加权：在滚动时效内，根据当前前置天数（Horizon）匹配客户专属的共识预测权重，替换通用 category_rolling_weight，更新 S&OP 对账指标。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_his_demand_header_rolling_weight 的 C++ DOD 物理对齐结构体
struct IpcHisDemandHeaderRollingWeightRecord {
    std::string header; // header 字符串 (-)
    std::string horizon; // horizon 字符串 (-)
    std::string consensus_forecast_weight; // consensus_forecast_weight 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。