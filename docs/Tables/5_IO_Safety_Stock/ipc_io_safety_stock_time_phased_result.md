---
table_name: "ipc_io_safety_stock_time_phased_result"
alias: "safety_stock_time_phased_result"
module: "5_IO_Safety_Stock"
cpp_struct: "SafetyStockTimePhasedResultRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_io_safety_stock_time_phased_result` (safety_stock_time_phased_result)

> **业务说明**: SafetyStockTimePhasedResult表报告了单级和多级安全库存项目的分时安全库存建议，这些安全库存项目被配置为生成分时结果(以及在安全库存项目下被带入多级家族的其他部分)。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `average_demand` | average_demand | `DECIMAL(18,2)` | Nullable | 历史需求的平均值(或历史预测的平均值)SafetyStockItemType.StandardDeviationDemandRule设置为“ForecastError”)。对于时间阶段的单梯队项目，它报告的是季节值的平均值。 |
| `future_demand` | future_demand | `DECIMAL(18,2)` | Nullable | 计算的未来需求值 |
| `caculated_service_level` | caculated_service_level | `DECIMAL(18,2)` | Nullable | 预期的服务水平百分比，使用有界安全库存计算。
如果SafetyStock = UnboundedSafetyStock，该值与
ServiceLevel相同。
取值为-1表示无法按照当前设置计算服务等级。如果SafetyStockItemType.TimePhasedProcessingRule设置为
" DaysOfSupplyForward "或" DaysOfSupplyBackward "，该字段的值设为-1。如果在人工台阶上允许使用安全库存，并且在面向客户的人工台阶上存在安全库存，则该值也设置为-1。 |
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
| `minimum_safety_stock` | minimum_safety_stock | `DECIMAL(18,2)` | Nullable | 安全库存的最低水平，以供应天数表示。
如果SafetyStockItemType。TimePhased
ProcessingRule被设置为“Ignore”和
SafetyStockItemType。中指定的值BoundsRule被设置为" DaysOfSupply "
SafetyStockTimePhasedBounds。MinimumDaysOfSupply在这里报告。
否则，该字段的值为-1。当这个字段中的值为负时，它被解释为负无穷大。

 |
| `material` | material | `VARCHAR` | PK / NOT NULL | - |
| `safety_stock` | safety_stock | `DECIMAL(18,2)` | Nullable | 根据历史数据计算的安全库存水平，并应用任何相关的界限规则。 |
| `service_level` | service_level | `DECIMAL(18,2)` | Nullable | 类中定义的服务级别
SafetyStockItem.ServiceLevel字段。
对于在一个或多个面向客户的项目下被带入多级家族的较低级别部件，该值是根据面向客户的项目的服务水平和标准偏差需求，以及每单位面向客户的项目所需的较低级别部件的数量来计算的。 |
| `standard_deviation_demand` | standard_deviation_demand | `VARCHAR` | Nullable | 计算历史需求的标准差(或历史预测误差的标准差)
SafetyStockItemType。StandardDeviation
DemandRule被设置为“ForecastError”)。 |
| `unbounded_ss` | unbounded_ss | `VARCHAR` | Nullable | 对于单梯队物品，这是没有任何限制的安全库存水平建议。
对于多级项，该字段的值与
如果找到满足服务水平和安全库存界限的解决方案，则返回SafetyStock。如果找不到这样的解决方案，则报告最低成本迭代的安全库存数量。这通常是在确保满足服务水平的同时，违反安全库存界限的次数最少的迭代。 |
| `current_ss` | current_ss | `VARCHAR` | Nullable | 零件的当前安全库存水平由Netting计算。
例如，这可能表示TimePhasedSafety表中提供的输入值，或者由覆盖范围逻辑计算的值。 |
| `date` | date | `VARCHAR` | Nullable | 此数据开始应用的日期 |
| `future_reorder_point` | future_reorder_point | `VARCHAR` | Nullable | 建议的再订货点，以维持建议的安全库存水平(基于平均未来需求)。对于多级安全库存项目，该数量受到中报告的详细信息的影响
SafetyStockItemFutureDemand表。 |
| `his_reorder_point` | his_reorder_point | `VARCHAR` | Nullable | 基于历史计算的Reorder Piont |
| `safety_stock_result` | safety_stock_result | `VARCHAR` | Nullable | Reference |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：MEIO 多级库存优化结果落库明细
* **因果流向**：该表记录了 MEIO 求解器运行后，在每一天推荐的优化安全库存水位。该结果直接输入至 MRP 模块，作为其时序逻辑的底层安全防线，用于触发计划订单（Planned Order）生成。
* **计算输出编排**：
  - 提取多级方差传导的最终成果，在此表写入每天对应的建议数量 `recommended_safety_stock`。
  - 记录当时的需求标准差 $\sigma_D$ 和最终求得的服务因子 $Z$，供计划员在前端看板对比分析，评估库存健康度。

###### 2. 物理内存结构设计 (C++ DOD Layout)
优化结果在内存中采用面向列的紧凑数组表示，便于后续 MRP 线程组进行只读无锁的并行水位消纳：
```cpp
// 对应 ipc_io_safety_stock_time_phased_result 的内存物理结构体
struct SafetyStockTimePhasedResultRecord {
    uint32_t part_id;                    // 物料ID
    uint32_t site_id;                    // 站点ID
    int day_bucket;                      // 计划相对天数
    double recommended_safety_stock;     // 推荐安全库存量 (对应 recommended_safety_stock)
    double demand_standard_deviation;    // 算出的时段需求标准差
    double calculated_service_factor_z;  // 算出的最终服务系数
};
```

###### 3. 边界与异常处理
* **极端波动保护**：若计算得到的建议安全库存为负数（由于反向分摊回溯产生浮点误差），引擎底座强制将其修正为 0，防止 MRP 时序出现异常可用量膨胀。