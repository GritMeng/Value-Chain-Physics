---
table_name: "ipc_io_safetystock_item_type"
alias: "safetystock_item_type"
module: "5_IO_Safety_Stock"
cpp_struct: "SafetyStockItemTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_io_safetystock_item_type` (safetystock_item_type)

> **业务说明**: SafetyStockItemType表包含控制设置，这些设置定义在生成安全库存建议时如何处理特定类型的安全库存项目。例如，此表中的字段控制项目详细信息，例如是否将项目配置为分时安全库存计算，以及如何计算历史需求的标准偏差。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `as_of_date_calendar` | as_of_date_calendar | `VARCHAR` | Nullable | Reference |
| `average_demand_rule` | average_demand_rule | `VARCHAR` | Nullable | 指示如何确定用于计算安全库存水平的平均需求值。
有效值为:
Mean - 这是默认值，通常适用于正态分布的需求，但是对于需求非正态分布的情况，也可以使用其他选项。
Median - 中位值
Mode - 取最小值. |
| `bounds_rule` | bounds_rule | `VARCHAR` | Nullable | - |
| `cycle_calendar` | cycle_calendar | `VARCHAR` | Nullable | 将项目配置为分阶段安全库存计算时使用的外部日历。参考日历通常应该为季节性/趋势数据定义一个完整的周期;例如，这可能是典型的年日历。
当使用Holt-Winters统计模型来估计需求的标准差时，此日历也适用 |
| `demand_outlier_rule` | demand_outlier_rule | `VARCHAR` | Nullable | Ignore - 不管
RemoveExcess - 移除超出部分
SmoothKeep - 移动后保留
SmoothRemove - 移动后移除超出部分 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |
| `forecast_outlier_rule` | forecast_outlier_rule | `VARCHAR` | Nullable | 
Ignore	
Remove	
RemoveExcess	 |
| `interval_calendar` | interval_calendar | `VARCHAR` | Nullable | - |
| `interval_per_period` | interval_per_period | `DECIMAL(18,2)` | Nullable | - |
| `lead_time_calendar` | lead_time_calendar | `VARCHAR` | Nullable | - |
| `lead_time_per_period` | lead_time_per_period | `VARCHAR` | Nullable | 指定IntervalsCalendar中的LeadTimeCalendar间隔数。例如，如果LeadTimeCalendar是
“Everyday”和IntervalsCalendar是“Week”，这可能被设置为“7”。
该字段的值用于保证安全库存计算中使用的需求参数的标准差 |
| `non_stationary_demand_rule` | non_stationary_demand_rule | `VARCHAR` | Nullable | 指定如何计算这些项目的历史需求的标准差，其中
TimePhasedProcessingRule设置为“Use”(适用于单级和多级项)。根据此设置计算的值然后用作生成分时安全库存建议时的参数。
Decomposition
HoltWinters
Manual
Simple |
| `period_calendar` | period_calendar | `VARCHAR` | Nullable | - |
| `period_per_cycle` | period_per_cycle | `VARCHAR` | Nullable | 对象中的PeriodCalendar单元的数目
CycleCalendar。例如，四个季度可能构成一个年度周期。
该领域适用于分阶段的安全库存计算，以及使用Holt-Winters方法计算基于预测误差变异性的需求标准差。 |
| `service_level_rule` | service_level_rule | `VARCHAR` | Nullable | 确定建议的安全库存水平所要达到的顾客满意程度。
该设置控制如何解释SafetyStockItem记录上指定的ServiceLevel值。服务水平以百分比值输入，并与此字段中的设置一起确定相应的z值或服务系数(在计算安全库存水平时用作乘数)

Cycle - 一种基于事件的服务级别，用于设置在交货时间内满足需求时不缺货的概率。这指的是alpha或type1服务级别。
FillRate - 一种以数量为基础的服务水平，它设定了应按时满足的需求的总体百分比。这指的是beta或type2服务级别。如果使用这个选项，SafetyStockItem记录上的MinimumOrderQuantity字段也应该被设置，用于确定给定填充率服务级别的z值。 |
| `standard_deviation_demand_rule` | standard_deviation_demand_rule | `VARCHAR` | Nullable | Crostons
DoubleES
ES
ForecastError
HoltWinters
LR
Manual
SD |
| `supply_variability` | supply_variability | `VARCHAR` | Nullable | 表明在计算安全库存水平和再订货点建议时，是否应考虑历史供应前置时间的可变性。如果要考虑历史供应提前期的可变性，则可以手动提供或根据历史供应数据计算所需的参数。
请注意，供应的可变性仅适用于计算单梯队物品的平稳(非时间阶段)安全库存
Ignore
Manual
Use |
| `time_phased_operation_rule` | time_phased_operation_rule | `VARCHAR` | Nullable | 指定是否为分阶段安全库存计算配置该项。对于单个和多个梯级的项目，都可以计算时间阶段的安全库存。该字段的设置也决定了如何计算需求参数的标准差。当设置为
“Use”，根据NonStationaryDemandRule字段中的设置计算需求的标准差。
的设置，计算需求的标准差StandardDeviationDemandRule字段
DOSBackward
DOSForward
Ignore
Use |
| `rolling_lead_time_demand` | rolling_lead_time_demand | `VARCHAR` | Nullable | Y - 用   N - 不用 |
| `safety_stock_item_type` | safety_stock_item_type | `VARCHAR` | PK / NOT NULL | 唯一值 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：安全库存策略属性模板映射
* **因果流向**：`ipc_io_safetystock_item_type` 定义了安全库存项目的基本策略属性模板（如：快销品、慢销品、战略备货品）。不同的模板类型决定了求解器在执行库存优化时，是否允许方差级联传导，以及默认的目标满足率（Fill Rate）。
* **模板决策逻辑**：
  - 读取物料的属性模板类型。
  - 若为“战略备货品（Strategic Buffer）”：禁用多级方差传导，强制执行本地静态安全天数覆盖。

###### 2. 物理内存结构设计 (C++ DOD Layout)
属性模板作为物料的常数元数据，在内存中以密集对齐结构存储：
```cpp
// 对应 ipc_io_safetystock_item_type 的 C++ 内存物理对齐结构体
struct SafetyStockItemTypeRecord {
    uint32_t item_type_id;            // 模板类型 ID (对应 item_type)
    double default_fill_rate_target;  // 默认目标交付率 (如 0.95)
    bool allow_variance_propagation;  // 是否允许方差级联传导
    uint8_t reorder_point_policy;     // 再订货点计算策略枚举
};
```