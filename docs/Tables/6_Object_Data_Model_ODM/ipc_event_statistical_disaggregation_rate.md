---
table_name: "ipc_event_statistical_disaggregation_rate"
alias: "event_statistical_disaggregation_rate"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcEventStatisticalDisaggregationRateRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_event_statistical_disaggregation_rate` (event_statistical_disaggregation_rate)

> **业务说明**: 此表用于分解受“事件管理”影响的统计预测项。它的结果和来自其他表的数据一起用于计算出现在EventStatisticalForecastDetailAdjustment表中的基于事件的统计预测调整列表。
为了计算统计预测项目的数量调整，首先使用为事件阶段指定的日历对EventStatisticalForecastDetail表中报告的受影响的预测细节进行分类。例如，如果事件阶段为每个月增加一定数量的预测数量，则使用月份日历重新存储受影响期间的预测详细信息。
对于在应用事件阶段之前没有预测数量的预测项目，也可以将记录添加到EventStatisticalDisaggregationRate表中
仅适用于单价调整的统计预测项目不包括在内EventStatisticalDisaggregationRate表。
这个表类似于EventDisaggregationRate表，它基于除统计预测外，还提供其他预测类别的详细表和报告结果

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `date` | date | `VARCHAR` | Nullable | - |
| `event_phase` | event_phase | `VARCHAR` | PK / NOT NULL | Reference |
| `event_phase_header` | event_phase_header | `VARCHAR` | Nullable | Reference |
| `qty` | qty | `VARCHAR` | Nullable | 数量 (Quantity) |
| `weight` | weight | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：事件统计分解率配置表。存储受营销事件影响的统计预测的分解比例，用于 Top-Down 时期的增量分摊。
* **计算逻辑编排**：
  1. 分解因子计算：计算促销期间各 SKU 的销售权重占比；2. 增量分摊：将大促总体活动目标 $Q_{promo}$ 自顶向下分解为 SKU 的天级增量，写入 EventStatisticalForecastDetailAdjustment 表。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_event_statistical_disaggregation_rate 的 C++ DOD 物理对齐结构体
struct IpcEventStatisticalDisaggregationRateRecord {
    std::string date; // date 字符串 (-)
    std::string event_phase; // event_phase 字符串 (Reference)
    std::string event_phase_header; // event_phase_header 字符串 (Reference)
    std::string qty; // qty 字符串 (数量 (Quantity))
    std::string weight; // weight 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。