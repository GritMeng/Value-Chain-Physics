---
table_name: "ipc_sop_disaggregation_rate_override"
alias: "disaggregation_rate_override"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcSopDisaggregationRateOverrideRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_sop_disaggregation_rate_override` (disaggregation_rate_override)

> **业务说明**: 此表提供了当将预测分解为详细级别时覆盖计算率的分解率。

如果此表中存在给定日期的特定零件客户和预测类别的记录，则该记录上的数量将用作确定该零件客户、类别和日期的预测分解的有效比率。如果在给定日期此表中不存在特定零件客户和预测类别的记录，则根据ForecastDisaggregationParameters表中提供的特定于零件客户和预测类别的参数(如果存在)计算分解率，或者使用场景中指定的默认分解参数
SOPAnalyticsConfiguration表).

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `date` | date | `VARCHAR` | Nullable | - |
| `header` | header | `VARCHAR` | PK / NOT NULL | HisDemandHeader |
| `override_qty` | override_qty | `VARCHAR` | Nullable | 用作按单位分解的比率的数量。数量是根据历史实际需求或一致预测计算的，这是由设置的值决定的
HistoricalDemandCategoryType.DisaggregationQuantityRule。中发现覆盖数量
ForecastDisaggregationOverride.数量，将在这里报告。 |
| `id` | id | `VARCHAR` | PK / NOT NULL | 唯一标识 |
| `unit_price` | unit_price | `VARCHAR` | Nullable | 被用来作为销售收入的Rate

取值顺序： 1. CustomerPrice.UnitPrice if Customer 有值
                   2. CustomerPrice.UnitPrice if Customer is null
                   3. Material.AverageSellingPrice
                   4.  = 0
If Material是AggregateMaterialCustomer的ComponentsM，不计算.
而AggregateMaterial需要计算，并且考虑其子节点权重. |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：S&OP 分解比例人工覆盖表。允许计划员手动输入特定物料客户的分配占比，强行阻断系统自动计算出的历史比例。
* **计算逻辑编排**：
  1. 覆盖替换：自顶向下分解时，系统优先检测该表是否存在覆盖行。若存在，强制采用 overridden_rate 分发预测，将其余残余量按归一化系数在其余 SKU 中进行平摊。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_sop_disaggregation_rate_override 的 C++ DOD 物理对齐结构体
struct IpcSopDisaggregationRateOverrideRecord {
    std::string date; // date 字符串 (-)
    std::string header; // header 字符串 (HisDemandHeader)
    uint32_t id; // id 逻辑ID/映射 (唯一标识)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。