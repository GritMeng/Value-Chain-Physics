---
table_name: "ipc_sop_disaggregation_rate"
alias: "disaggregation_rate"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcSopDisaggregationRateRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_sop_disaggregation_rate` (disaggregation_rate)

> **业务说明**: 报告物料-客户和预测类别组合的一般分解率.
其提供的默认参数通常计算此表中的比率
SOPConfiguration表或预测类别中提供的具体参数
DisaggregationParameters表或在DisaggregationParametersByCategory表中提供的部分客户和预测类别特定参数。还有以下几点需要考虑:

如果HistoricalDemandCategoryType表上的DisaggregationQuantityRule字段设置为
“Actuals”，ForecastDisaggregationOverride表可用于指定特定日期的特定物料客户和预测类别的费率，然后这些将在
DisaggregationRateByPartCustomer表，而不是为该日期的部分客户和类别计算它们。但是，如果PartCustomer.DisaggregationStartDate和
PartCustomer.DisaggregationEndDate设置，则忽略速率覆盖。



| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `category` | category | `VARCHAR` | Nullable | Reference : HisDemandCategory |
| `date` | date | `VARCHAR` | Nullable | - |
| `eff_unit_price` | eff_unit_price | `VARCHAR` | Nullable | 被用来作为销售收入的Rate

取值顺序： 1. CustomerPrice.UnitPrice if Customer 有值
                   2. CustomerPrice.UnitPrice if Customer is null
                   3. Material.AverageSellingPrice
                   4.  = 0
If Material是AggregateMaterialCustomer的ComponentsM，不计算.
而AggregateMaterial需要计算，并且考虑其子节点权重. |
| `header` | header | `VARCHAR` | Nullable | HisDemandHeader |
| `material_customer` | material_customer | `VARCHAR` | PK / NOT NULL | - |
| `qty` | qty | `VARCHAR` | Nullable | 数量 (Quantity) |
| `override_qty` | override_qty | `VARCHAR` | Nullable | 按单位分解的默认数量，它是根据历史实际需求或一致预测计算的。中发现覆盖数量
ForecastDisaggretationOverride。数量，这里没有报告。 |
| `inner_calendar` | inner_calendar | `VARCHAR` | Nullable | 内部日历用于具有季节趋势的预测分解，以定义季节的长度。该日历由外部日历划分，以提供用于确定预测分解到哪个时期的索引。
例如，对于季节按月按年分解，这将被设置为每月日历。具有较高数值的月份将收到更多被分解的数据。如果预测分解不是季节性的，则应将其设置为与外部日历相同的值 |
| `outer_calendar` | outer_calendar | `VARCHAR` | Nullable | 外部分解日历用于定义分解预测的期间。例如，使用季节按月按年分解，这将被设置为年度日历。
此字段引用的日历不能表示比内部日历引用的日历更小的时间间隔(如果分解不是季节性的，它们可以引用相同的日历)。外部日历标记也应始终直接落在内部日历标记上。例如，使用按月分解，年度(外部)日历标记应该落在标记上 |
| `override_category` | override_category | `VARCHAR` | Nullable | 支持按预测类别的默认分解覆盖率。使用覆盖率代替计算的分解率(默认分解率或特定部分-客户和类别的分解率)。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：S&OP 动态分解比例计算结果表。存储系统计算出的各 SKU-Site 占产品族总体预测的分摊百分比（Rate），供 disaggregation 算法直接读取。
* **计算逻辑编排**：
  1. 分解占比计算：对产品族下属各 SKU，计算其历史有效销量的占比：\n      $$ Rate_i = \\frac{{Historical\_Sales\_Qty_i}}{{\\sum_k Historical\_Sales\_Qty_k}} $$\n   2. 归一化校验：验证 $\\sum Rate_i = 1.0$；3. 级联分摊：在 forecast_detail 写入时，直接读取此表的占比乘以大盘预测总量进行分摊落库。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_sop_disaggregation_rate 的 C++ DOD 物理对齐结构体
struct IpcSopDisaggregationRateRecord {
    std::string category; // category 字符串 (Reference : HisDemandCategory)
    std::string date; // date 字符串 (-)
    std::string header; // header 字符串 (HisDemandHeader)
    uint32_t material_customer; // material_customer 逻辑ID/映射 (-)
    std::string qty; // qty 字符串 (数量 (Quantity))
    uint32_t override_category; // override_category 逻辑ID/映射 (支持按预测类别的默认分解覆盖率。使用覆盖率代替计算的分解率(默认分解率或特定部分-客户和类别的分解率)。)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。