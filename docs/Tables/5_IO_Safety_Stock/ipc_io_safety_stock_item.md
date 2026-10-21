---
table_name: "ipc_io_safety_stock_item"
alias: "safety_stock_item"
module: "5_IO_Safety_Stock"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_io_safety_stock_item` (safety_stock_item)

> **业务说明**: SafetyStockItem表用于配置单级和多级安全库存项。该表中的每条记录都确定了应对其提出安全库存建议的特定项目(物料)，指定该项目是用于单级还是多级安全库存计算，并包含适用于单级、多级或两种计算的其他参数。
TimePhasedSafetyStock和SafetyStock函数都使用单级安全库存项，根据历史数据为单个部件生成安全库存和再订货点建议，以满足指定的服务级别(可选地，预测数据也可用于生成面向未来的再订货点建议)。多级安全库存项目由一种算法使用，该算法在零件网络中生成安全库存建议，以便既满足面向客户的终端项目定义的服务水平，同时，通过在网络中推荐安全库存的战略布局，试图将库存的总持有成本降至最低。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `average_demand` | average_demand | `DECIMAL(18,2)` | Nullable | 表示在安全库存计算中用于该部件的平均历史需求的值。
只适用于非时间阶段的项目
类型。StandardDeviationDemandRule设置为“Manual”。
当一个项目的可用历史数据有限时，可以手动提供平均需求，但是它的平均需求和其他参数是已知的或通过其他方式估计的。
注意，平均需求应该表示为
Type.IntervalsCalendar。 |
| `outlier_mawindow` | outlier_mawindow | `VARCHAR` | Nullable | 历史需求或预测误差的移动平均窗口的长度。
此字段仅适用于下列项目
OutlierType。datarrule设置为“MovingAverageError”。
方法指定的值来解释阈值
DemandOutlierThreshold设置。
Default:3 |
| `outlier_threshold` | outlier_threshold | `VARCHAR` | Nullable | 阈值。与需求数量或预测误差点的平均值的(最小)标准差数必须被视为离群值。然后根据RemoveDemandOutliers字段中的设置，删除、平滑或忽略检测到的任何异常值。
使用此设置时，根据指定的OutlierType.DetectionRule解释阈值。例如，如果DetectionRule = "Winsorizing"，那么检测到的离群值将被标识为在有序数据中指定百分位数之外的数量。
Defult : 3 |
| `outlier_type` | outlier_type | `VARCHAR` | Nullable | Reference |
| `safter_interval` | safter_interval | `VARCHAR` | Nullable | Forward的时间长度，在backward之后 |
| `sbefore_interval` | sbefore_interval | `VARCHAR` | Nullable | Backward的是简单长度 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |
| `lag` | lag | `VARCHAR` | Nullable | 类型数量。AsOfDateCalendar间隔，以便在收集用于预测误差计算的历史预测数据时进行回顾。
例如，假设这个值被设置为“1”，并且
AsOfDateCalendar设置为“Month”。在这种情况下，
HisDemandSeries一个月的实际记录与参考a的详细预测记录进行比较
其中AsOfDate在前一个月。类似地，值“2”表示将一个月的实际情况与两个月前的预测进行比较。 |
| `future_average_demand_profile` | future_average_demand_profile | `VARCHAR` | Nullable | 一种基于交货期倍数和其他设置的配置文件的参考，它定义了在确定用于再订货点计算的平均未来需求值时使用的范围。如果所有未来的数据点都用来计算未来的平均需求，这个引用是可空的，可以留空。
请注意，此字段仅支持适用于单梯次安全库存项目的再订购点计算 |
| `future_interval_count` | future_interval_count | `VARCHAR` | Nullable | IntervalCalendar的数量，在运行日期之后，应该收集当前/预测需求数据以确定未来的平均需求.
一下作用：
1. 用于计算SafetyStockItemFutureDemand表的细节SafetyStockItemFutureDemand表用于计算单梯次安全库存项目的供应天数。该表还用于计算未来的平均需求，这将有助于确SafetyStockTimePhasedResult。多级安全库存项目的FutureReorderPoint。
2.反映在报告SafetyStockResult.AverageFutureDemand的数量上，它有助于确定单级安全库存项目的再订货点计算 |
| `his_average_demand_profile` | his_average_demand_profile | `VARCHAR` | Nullable | 对一个概要文件的引用，基于交货时间倍数和其他设置，它定义了在确定用于再订购点计算的平均历史需求值时使用的范围。如果应该使用所有历史数据点来计算平均历史需求，则此引用可为空，并且可以留空。 |
| `his_demand_category` | his_demand_category | `VARCHAR` | Nullable | 参考历史需求类别，从中收集实际数据以供使用在安全库存计算 |
| `his_end_date` | his_end_date | `VARCHAR` | Nullable | 收集历史的数据的截止日期 |
| `his_forecast_category` | his_forecast_category | `VARCHAR` | Nullable | 对历史预测类别的参考。在计算预测误差的标准差时，历史预测记录在
只有当通过它们的HistoricalDemandSeriesDetail系列类别参考。
此字段仅适用于下列项目类型。StandardDeviationDemandRule设置为
" ForecastError "并键入。TimePhasedProcessingRule是“Ignore”或“DaysOfSupplyBackward”或
“DaysOfSupplyForward”。 |
| `his_leading_zero` | his_leading_zero | `VARCHAR` | Nullable | 初始数量为零 |
| `his_start_date` | his_start_date | `DATE` | Nullable | 开始收集历史数据的日期 |
| `his_supply_category` | his_supply_category | `DECIMAL(18,2)` | Nullable | 参考历史供应类别，从中
应收集实际数据用于确定平均交货时间(仅在以下情况下使用)
类型。SupplyVariabilityRule被设置为“Use”)。此引用可为空，在安全库存计算不需要考虑交货时间可变性的情况下，可以将其保留为空。
请注意，此字段和历史供应计算仅适用于单梯队安全库存计算。 |
| `lead_time` | lead_time | `DECIMAL(18,2)` | Nullable | 用于单梯次安全库存计算的零件的标准平均提前期值。如果适用
类型。SupplyVariabilityRule被设置为“Ignore”或
“manual”，或者如果没有可用的历史供应数据
(否则，交货期根据历史供应数据计算)。
这个值应该主要用。来表示
类型。LeadTimeCalendar单位，只表示为
类型。未定义的intervalcalendar单位。 |
| `lead_time_outlier_moving_average_window` | lead_time_outlier_moving_average_window | `DECIMAL(18,2)` | Nullable | 历史提前期的移动平均窗口的长度。
此字段仅适用于下列项目
OutlierType.datarrule设置为“MovingAverageError”。
方法指定的值来解释阈值
LeadTimeOutlierThreshold设置 |
| `lead_time_outlier_threshold` | lead_time_outlier_threshold | `VARCHAR` | Nullable | LeadTime 的离异值/阈值 |
| `lead_time_outlier_type` | lead_time_outlier_type | `VARCHAR` | Nullable | Reference:OutlierType |
| `lead_time_safter_interval_count` | lead_time_safter_interval_count | `VARCHAR` | Nullable | Forward的时间间隔，在backward之后 |
| `lead_time_sbefore_interval_count` | lead_time_sbefore_interval_count | `VARCHAR` | Nullable | Backward的时间间隔 |
| `order_qty` | order_qty | `VARCHAR` | Nullable | 如果ServiceLevelRule设置为“FillRate”，指定了在确定安全库存计算中使用的服务系数时用作输入的最小订单数量值。 |
| `material` | material | `VARCHAR` | Nullable | Reference:Material |
| `operation_rule` | operation_rule | `VARCHAR` | Nullable | MultiEchelon
SingleEchelon |
| `remove_demand_outlier` | remove_demand_outlier | `VARCHAR` | Nullable | 确定是否应该从数据序列中删除历史数据中的异常值(减少序列的长度，而不是在不改变序列长度的情况下调整异常值)。
DemandOutlierThreshold设置定义了项目数量在被视为离群值之前与平均值之间的标准偏差数:
No - 不移除。 遵循OutlierType的设置调整
Yes - 移除
Low - 移除低值
High - 移除高值

 |
| `service_level` | service_level | `VARCHAR` | Nullable | 计算出的零件安全库存建议的服务水平百分比.对于多级项目，这总是指定订单完全满足的概率(而不是备货)，对于单级项目，该字段的解释是可配置的，具体取决于Type.ServiceLevelRule设置如下:
Cycle - 这指定了订单被完全满足的概率(没有缺货).
FillRate - 这指定了应该按时满足的需求的百分比. |
| `service_time` | service_time | `VARCHAR` | Nullable | 这个项目应该总是能够满足的服务或交货时间。
此字段仅适用于多级安全库存项目，并定义从客户订购项目到项目发货之间的允许时间。该字段中的值应该用每天。因此，计算出的安全库存水平将确保在这里指定的日历天数内满足该项目的订单。例如，对于在面向客户的位置保存的部件，通常可以将其设置为0 |
| `standard_deviation_demand` | standard_deviation_demand | `VARCHAR` | Nullable | 手动 |
| `standard_deviation_lead_time` | standard_deviation_lead_time | `VARCHAR` | Nullable | 手动 |
| `safetystock_item_type` | safetystock_item_type | `VARCHAR` | Nullable | 用于此安全库存项的处理规则。例如，此参考设置用于安全库存计算的日历，指定如何计算标准偏差，并定义如何处理需求异常值。 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多级安全库存优化（MEIO）与方差级联传导
* **因果流向**：`ipc_io_safety_stock_item` 存储安全库存优化的策略配置。系统利用多级库存优化（MEIO）算法在全网拓扑中进行库存战略排布。
* **方差传导与服务水平优化编排**：
  1. 需求方差自底向上聚合：引擎沿有向无环图逆向传播需求波动（方差 $\sigma_{D}^2$）和提前期波动（方差 $\sigma_{L}^2$），推演上游零部件的合成需求方差：
     $$\sigma_{Total, i, t} = \sqrt{ L_i \cdot \sigma_{D, i, t}^2 + D_{i, t}^2 \cdot \sigma_{L, i}^2 }$$
  2. 延迟服务时间优化（Service Time Optimization）：算法评估在不同梯队节点（原材料、半成品、成品）保留库存的持有成本（Holding Cost），自动拉伸或缩短各节点的承诺服务时间（Service Time），寻找系统总持有成本最低的库存储备方案。
  3. 安全库存下发：计算出的最终安全库存指标写入结果表，同步作用于 IOP 阶段，作为 MRP 水位冲抵的刚性安全门槛。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存中，为了能在数百万个 SKU 节点上实现秒级的库存策略优化，IPC 采用**时序二叉段树（Segment Tree）**对计划时间轴进行剖分，利用 OpenMP 进行并发的 **规约合并算子 $\oplus$** 规约计算：

```cpp
// 用于并行规约时序波动的二叉段树节点结构 (64字节高速缓存行对齐)
struct alignas(64) SegmentNode {
    double mean_demand = 0.0;       // 区间平均需求均值
    double variance_demand = 0.0;   // 区间需求波动方差
    int start_day;                  // 时序区间起点
    int end_day;                    // 时序区间终点
};

// 段树节点规约合并算子 (由 OpenMP 核心线程并行调用，复杂度 O(log N))
inline SegmentNode reduce_segments(const SegmentNode& a, const SegmentNode& b) {
    SegmentNode merged;
    merged.mean_demand = a.mean_demand + b.mean_demand;
    merged.variance_demand = a.variance_demand + b.variance_demand; // 独立正态增量相加
    merged.start_day = a.start_day;
    merged.end_day = b.end_day;
    return merged;
}
```