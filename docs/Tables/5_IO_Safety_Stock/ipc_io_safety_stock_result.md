---
table_name: "ipc_io_safety_stock_result"
alias: "safety_stock_result"
module: "5_IO_Safety_Stock"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_io_safety_stock_result` (safety_stock_result)

> **业务说明**: SafetyStockResult表根据单层和多层安全库存项目(以及进入多层家族的其他部分)的历史数据报告单个安全库存建议。此外，在此表中报告了用于确定安全库存的一些计算参数.
请注意，本表中报告的值是用
AnalyticConfiguration.MEIOReportCalendar。因此，可以应用日历转换来确保计算的参数和结果按照该日历表示(从SafetyStockItemType转换为单梯次结果)。将IntervalsCalendar转换为MEIOReportCalendar，并将每日日历的多级结果转换为MEIOReportCalendar。
如果安全库存项目被配置为报告时间阶段的安全库存值，或者如果需要推荐的历史和/或未来的重新排序点，则可以使用SafetyStockTimePhasedResult表

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `average_demand` | average_demand | `DECIMAL(18,2)` | Nullable | 历史需求的平均值(或历史预测的平均值)SafetyStockItemType.StandardDeviationDemandRule设置为“ForecastError”)。对于时间阶段的单梯队项目，它报告的是季节值的平均值。 |
| `average_future_demand` | average_future_demand | `DECIMAL(18,2)` | Nullable | 未来(当前/预测)需求的平均值。
1. 对于单梯队项目，这是在项目的平均需求配置文件中收集的未来需求的平均值(如果没有指定配置文件，则为所有未来需求)。此外，该字段还考虑了来自的值
SafetyStockItem.FutureIntervalCount。
2. 对于多级项目，这是基于需求的
MEIOReportCalendar和MEIOFutureIntervalCount(两个字段都来自AnalyticConfiguration表) |
| `caculated_service_level` | caculated_service_level | `DECIMAL(18,2)` | Nullable | 预期的服务水平百分比，使用有界安全库存计算。
如果SafetyStock = UnboundedSafetyStock，该值与
ServiceLevel相同。
取值为-1表示无法按照当前设置计算服务等级。如果SafetyStockItemType.TimePhasedProcessingRule设置为
" DaysOfSupplyForward "或" DaysOfSupplyBackward "，该字段的值设为-1。如果在人工台阶上允许使用安全库存，并且在面向客户的人工台阶上存在安全库存，则该值也设置为-1。 |
| `calendar_conversion_rate` | calendar_conversion_rate | `DECIMAL(18,2)` | Nullable | 在此表中报告值时，部分使用的适用日历转换率。
对于多级项，这是Everyday日历和MEIOReportCalendar之间的比率。对于单梯队项目，这是SafetyStockItem之间的比率。intervalcalendar和meoreportcalendar. |
| `early_arrival_stock` | early_arrival_stock | `DECIMAL(18,2)` | Nullable | 这部分的提前到货量。提前到货库存是指在相关客户订单发货之前到达某个地点的补货订单。当一个阶段的出库服务时间超过其入库服务时间时，提前到货库存可能会发生在有提前期可变性的多级零件上。然后，多级优化计算试图最小化安全库存和提前到达库存的总持有成本。 |
| `incoming_service_time` | incoming_service_time | `DECIMAL(18,2)` | Nullable | 在多级系列中，计算所有级/部件的最大使用时间。
这是该阶段满足其直接需求(例如，从其组件供应)的最大天数。表示为
MEIOReportCalendar |
| `interval_counts_of_supply` | interval_counts_of_supply | `DECIMAL(18,2)` | Nullable | 该记录上报告的SafetyStock值可以满足的MEIOReportCalendar需求间隔的数量。 |
| `lead_time` | lead_time | `DECIMAL(18,2)` | Nullable | 零件的交货时间(基于输入值或根据历史供应计算)。注意，该值表示为
MEIOReportCalendar的时间间隔(从多级项的Everyday日历转换，或从
SafetyStockItem.Intervals 单梯队项目的间隔日历)。 |
| `maximum_days_of_supply` | maximum_days_of_supply | `DECIMAL(18,2)` | Nullable | 如果SafetyStockItemType.TimePhased
OperationRule被设置为“Ignore”和
SafetyStockItemType中指定的值BoundsRule被设置为" DaysOfSupply "
SafetyStockTimePhasedBounds.MaximumDaysOfSupply在这里报告.


 |
| `minimum_days_of_supply` | minimum_days_of_supply | `DECIMAL(18,2)` | Nullable | 安全库存的最低水平，以供应天数表示。
如果SafetyStockItemType。TimePhased
ProcessingRule被设置为“Ignore”和
SafetyStockItemType。中指定的值BoundsRule被设置为" DaysOfSupply "
SafetyStockTimePhasedBounds。MinimumDaysOfSupply在这里报告。 |
| `maximum_safety_stock` | maximum_safety_stock | `DECIMAL(18,2)` | Nullable | 安全库存的最高水平，以数量表示。
l如果SafetyStockItemType。TimePhased
OperationRule被设置为“Ignore”和
SafetyStockItemType.BoundsRule被设置为“Qty”
SafetyStockTimePhasedBounds.MaximumQty在这里报告。
如果SafetyStockItemType.TimePhasedProcessingRule设置为
“Ignore”并且SafetyStockItemType.BoundsRule设置为
“DaysOfSupply”，每天的平均需求量乘以SafetyStockTimePhasedBoundsRule.MaximumDaysOfSupply结果在这里报告。
否则，该字段的值为-1 |
| `meiofamily` | meiofamily | `DECIMAL(18,2)` | Nullable | Reference |
| `minimum_safety_stock` | minimum_safety_stock | `DECIMAL(18,2)` | Nullable | 安全库存的最低水平，以供应天数表示。
如果SafetyStockItemType。TimePhased
ProcessingRule被设置为“Ignore”和
SafetyStockItemType。中指定的值BoundsRule被设置为" DaysOfSupply "
SafetyStockTimePhasedBounds。MinimumDaysOfSupply在这里报告。
否则，该字段的值为-1。当这个字段中的值为负时，它被解释为负无穷大。

 |
| `outgoing_service_time` | outgoing_service_time | `DECIMAL(18,2)` | Nullable | 家庭中这一阶段的推荐服务时间。这是零件保证能满足要求的最大天数。
对于多级安全库存项目，将其设置为安全库存项目上指定的输入值(如果提供的输入服务时间大于部件的累积提前期，则使用累积提前期值)。对于多级家族中低于这些终端项目的所有部件，其计算目标是使总库存持有成本最小化。 |
| `material` | material | `VARCHAR` | Nullable | - |
| `material_source` | material_source | `VARCHAR` | Nullable | - |
| `report_calendar` | report_calendar | `DECIMAL(18,2)` | Nullable | 对日历的引用，此记录上的值以其间隔表示。这将返回与AnalyticsConfiguration中引用的相同的日历MEIOReportCalendar |
| `safety_stock` | safety_stock | `DECIMAL(18,2)` | Nullable | 根据历史数据计算的安全库存水平，并应用任何相关的界限规则。 |
| `service_level` | service_level | `DECIMAL(18,2)` | Nullable | 类中定义的服务级别
SafetyStockItem.ServiceLevel字段。
对于在一个或多个面向客户的项目下被带入多级家族的较低级别部件，该值是根据面向客户的项目的服务水平和标准偏差需求，以及每单位面向客户的项目所需的较低级别部件的数量来计算的。 |
| `standard_deviation_demand` | standard_deviation_demand | `VARCHAR` | Nullable | 计算历史需求的标准差(或历史预测误差的标准差)
SafetyStockItemType。StandardDeviation
DemandRule被设置为“ForecastError”)。 |
| `standard_deviation_lead_time` | standard_deviation_lead_time | `VARCHAR` | Nullable | 计算历史供应提前期的标准差 |
| `unbounded_ss` | unbounded_ss | `VARCHAR` | Nullable | 对于单梯队物品，这是没有任何限制的安全库存水平建议。
对于多级项，该字段的值与
如果找到满足服务水平和安全库存界限的解决方案，则返回SafetyStock。如果找不到这样的解决方案，则报告最低成本迭代的安全库存数量。这通常是在确保满足服务水平的同时，违反安全库存界限的次数最少的迭代。 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多级安全库存动态分摊与二次分配
* **因果流向**：`ipc_io_safety_stock_result` 存储库存优化的最终建议值。该结果被 IOP 级消纳引擎加载，映射为水位轴的保护性屏障。
* **二次分配优化编排**：
  在存在交叉替代料（如物料 $A_1$ 既能替代组 1，又能替代组 2 的短缺）时，系统执行二次安全库存动态平衡：
  1. 历史消耗锚定：计算 $T-1$ 期内替代料在各方向上的真实消纳比率，锁定基准占比。
  2. 方差补偿分摊：在 $T$ 时段，若组 1 的实际方差 $\sigma_1^2$ 下降，溢出安全库存盈余 $\Delta SS_1$。系统自动将该盈余向波动加剧的组 2 进行二次转移（Residual Reallocation），实现物理库存的动态自适应平抑。