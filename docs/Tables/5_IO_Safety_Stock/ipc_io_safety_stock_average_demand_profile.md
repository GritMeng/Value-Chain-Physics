---
table_name: "ipc_io_safety_stock_average_demand_profile"
alias: "safety_stock_average_demand_profile"
module: "5_IO_Safety_Stock"
cpp_struct: "AverageDemandProfileRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_io_safety_stock_average_demand_profile` (safety_stock_average_demand_profile)

> **业务说明**: SafetyStockAverageDemandProfile包含可选的概要文件，可用于定义安全库存项目的历史和/或未来需求收集的范围，以用于计算平均需求。这些是平均需求计算，用作确定推荐的历史和未来再订购点的输入
此表中的每个配置文件由一个Offset值组成，该值定义了收集数据的起点或终点，一个Multiplier值定义了收集历史数据的项目前置时间的倍数，以及一个Extend值，该值向收集数据的期间添加了一个固定值。这三个字段的具体用法取决于概要文件是用于收集历史需求还是收集未来需求。
当使用配置文件确定收集历史需求的窗口时，此表中提供的值与安全库存项目上定义的历史收集间隔的最后日期以及项目的平均前置时间一起使用，以定义配置文件的开始和结束日期，
如下所示:
1. Historical Start :LastDate - Offset - CalcAverageLeadTime * Multiplier -Extend
2. Historical End :LastDate -Offset
相反，当使用概要文件确定收集未来需求的窗口时，则使用此表中提供的值与运行日期和安全库存项目的平均提前期一起定义概要文件的开始和结束日期，如下所示:
1. Future Start: RunDate + Offset
2. Future End: RunDate + Offset + CalcAverageLeadTime * Multiplier + Extend



| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `id` | id | `VARCHAR` | PK / NOT NULL | - |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |
| `extend` | extend | `VARCHAR` | Nullable | 用的表示的固定数量的间隔
SafetyStockItem.IntervalsCalendar，用于增加或扩展计算平均需求的范围(在考虑偏移值并应用前置时间乘数之后) |
| `multiplier` | multiplier | `VARCHAR` | Nullable | 应用于安全库存项目的平均交货时间的乘数，其乘积然后用于确定计算平均需求的窗口大小 |
| `offset` | offset | `VARCHAR` | Nullable | 用的表示的固定数量的间隔
SafetyStockItem.IntervalsCalendar，以抵消用于收集平均需求计算中使用的需求的Horizon的开始/结束。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多时窗滑动平均需求与提前期变异度分析
* **因果流向**：安全库存计算极其依赖于对“平均日需求”的精确估计。`ipc_io_safety_stock_average_demand_profile` 定义了计算该平均需求的时窗长度（如：是取历史过去 90 天，还是取未来 30 天预测，或者按提前期的 N 倍长度进行动态伸缩）。
* **滑动时窗编排**：
  1. 窗口起点终点推算：根据配置的 Offset、Multiplier 和 Extend 天数，计算滑动窗口。例如未来需求窗口：
     $$ Start = RunDate + Offset $$
     $$ End = RunDate + Offset + LeadTime \times Multiplier + Extend $$
  2. 平均销量与变异度核算：在该滑动时窗内累加需求，除以总天数得到平均日需求 $\mu_D$，并计算样本标准差 $\sigma_D$，作为安全库存方差传导的底座输入。

###### 2. 物理内存结构设计 (C++ DOD Layout)
配置项在 C++ 引擎中表现为紧凑的参数块，由时序数轴处理器直接读取：
```cpp
// 对应 ipc_io_safety_stock_average_demand_profile 的内存物理结构
struct AverageDemandProfileRecord {
    uint32_t profile_id;            // 配置文件逻辑ID
    int offset_days;                // 相对起始日偏移天数 (对应 Offset)
    double lead_time_multiplier;    // 提前期倍数值 (对应 Multiplier)
    int extend_days;                // 额外固定延长天数 (对应 Extend)
};
```

###### 3. 边界与异常处理
* **超出计划期截断**：如果算出的未来窗口结束日期 $End$ 超出了主计划期（Planning Horizon）的终点，引擎会自动将窗口截断在主计划期末端，并根据实际覆盖天数对分母进行重归一化，防止产生指针越界或计算得出空值。