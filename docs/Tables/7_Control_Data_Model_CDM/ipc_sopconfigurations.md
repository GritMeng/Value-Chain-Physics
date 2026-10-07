---
table_name: "ipc_sopconfigurations"
alias: "sopconfigurations"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcSopconfigurationsRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_sopconfigurations` (sopconfigurations)

> **业务说明**: 用于定义预测和分解率计算的默认参数。它只保存一条使用SOP填充的记录
控制表工作簿中的分析配置工作表。这个记录可以在一个场景接一个场景的基础上进行修改，允许您使用这个表对各种统计预测和分解场景进行建模。

请注意，该表中的许多分解参数可以通过在列表中提供值来覆盖特定的零件客户和预测类别组合ForecastDisaggregationParameters表中提供的值可以覆盖与给定预测类别关联的所有零件客户
ForecastDisaggregationParametersByCategory表.

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `cal_forecast_start_date` | cal_forecast_start_date | `VARCHAR` | Nullable | 开始报告分解率(含)的计算日期。
ForecastStartOffset中取正值，然后计算为calhistoricalenddate加
(被)那个数目的CycleCalendar间隔抵消。否则，这就等于
CalcHistoricalEndDate |
| `calendar` | calendar | `VARCHAR` | Nullable | - |
| `disaggregation_actual_category` | disaggregation_actual_category | `VARCHAR` | Nullable | Reference: HisDemandCategory |
| `disaggregation_calendar` | disaggregation_calendar | `VARCHAR` | Nullable | 分解日历是用于所有分解的唯一传播间隔。它定义了订单可以分解成的时间段，因此可以在分解日历级别输入和存储预测细节。它应该是处理预测数量所需的最小桶。典型值包括周和月.
请注意，此值可以在该部分由客户层面提供的有效引用
PartCustomer.DisaggregationCalendar
字段覆盖。否则，如果该引用留下
对于给定的零件客户，值为Null
使用该字段中提供的。 |
| `cal_forecast_end_date` | cal_forecast_end_date | `VARCHAR` | Nullable | 停止报告分解率的计算日期(不排除)。
如果ForecastEndOffset中提供的正值，然后计算为CalcForecastStartDate加上(偏移量)CycleCalendar间隔的数量。
否则，它被计算为CalcForecastStartDate加1CycleCalendar区间。 |
| `cal_historical_end_date` | cal_historical_end_date | `VARCHAR` | Nullable | 可以收集历史数据的最后计算日期。例如，这定义了可以收集历史数据以用于确定分解率的日期。
设置为运行日期当天或之前最接近的CycleCalendar间隔(例如，当前月初) |
| `cycle_calendar` | cycle_calendar | `VARCHAR` | Nullable | 反映用于计算历史结束日期、预测开始日期和预测结束日期的s&p周期的日历。也用于定义抵抗区域和预测与最佳拟合计算相关的滞后。这通常是一个月的日历。 |
| `disaggregation_historical_interval_count` | disaggregation_historical_interval_count | `VARCHAR` | Nullable | 内部日历周期的数量在预报开始日期之前使用
收集历史数据和因果关系用于计算分解的因素
利率。  收集历史预测的时间= CalHistoricalEndDate + 此值 |
| `rundate` | rundate | `VARCHAR` | Nullable | Calendar |
| `forecast_end_offset` | forecast_end_offset | `VARCHAR` | Nullable | CalHistoricalEndDate + 此值 = CalForecastStartDate |
| `forecast_start_offset` | forecast_start_offset | `VARCHAR` | Nullable | CalForecastStartDate + 此值= CalForecastEndDate |
| `disaggregation_override_category` | disaggregation_override_category | `VARCHAR` | Nullable | 支持按预测类别的默认分解覆盖率。使用覆盖率代替计算的分解率(默认分解率或特定物料-客户和类别的分解率)。 |
| `disaggregation_inner_calendar` | disaggregation_inner_calendar | `VARCHAR` | Nullable | - |
| `disaggregation_outer_calendar` | disaggregation_outer_calendar | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：S&OP 全局运营策略配置表。定义 S&OP 计算流程中的系统级核心常数，如预测重平衡开关、共识算法激活状态、财务期初汇率集等。
* **计算逻辑编排**：
  1. 求解器全局变量初始化：在 S&OP 模块载入时，读取配置初始化引擎控制句柄；2. 流程拦截：若 rebalance_enabled 为 false，直接跳过供应需求再平衡迭代，保留常规共识预测结果。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_sopconfigurations 的 C++ DOD 物理对齐结构体
struct IpcSopconfigurationsRecord {
    std::string calendar; // calendar 字符串 (-)
    std::string disaggregation_actual_category; // disaggregation_actual_category 字符串 (Reference: HisDemandCategory)
    std::string cycle_calendar; // cycle_calendar 字符串 (反映用于计算历史结束日期、预测开始日期和预测结束日期的s&p周期的日历。也用于定义抵抗区域和预测与最佳拟合计算相关的滞后。这通常是一个月的日历。)
    std::string rundate; // rundate 字符串 (Calendar)
    std::string forecast_end_offset; // forecast_end_offset 字符串 (CalHistoricalEndDate + 此值 = CalForecastStartDate)
    std::string forecast_start_offset; // forecast_start_offset 字符串 (CalForecastStartDate + 此值= CalForecastEndDate)
    uint32_t disaggregation_override_category; // disaggregation_override_category 逻辑ID/映射 (支持按预测类别的默认分解覆盖率。使用覆盖率代替计算的分解率(默认分解率或特定物料-客户和类别的分解率)。)
    std::string disaggregation_inner_calendar; // disaggregation_inner_calendar 字符串 (-)
    std::string disaggregation_outer_calendar; // disaggregation_outer_calendar 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。