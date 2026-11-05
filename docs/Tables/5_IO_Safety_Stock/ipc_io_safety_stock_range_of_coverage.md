---
table_name: "ipc_io_safety_stock_range_of_coverage"
alias: "safety_stock_range_of_coverage"
module: "5_IO_Safety_Stock"
cpp_struct: "RangeOfCoverageRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_io_safety_stock_range_of_coverage` (safety_stock_range_of_coverage)

> **业务说明**: 定义计算平均需求的范围使用“DaysOfSupplyForward”或“DaysOfSupplyBackward”处理规则计算零件推荐安全库存的目的


| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `count` | count | `INTEGER` | Nullable | 计算平均需求的间隔数。对于多级安全库存项目，间隔是每日日历上的天数。对于单级库存指定的日历上的天数为间隔SafetyStockItemType.IntervalsCalendar字段。 |
| `offset` | offset | `INTEGER` | Nullable | 相对于计算安全库存的日期，计算平均需求的日期范围的开始。
对于多级安全库存项目，此值是“Everyday”的天数。对于单级安全库存为SafetyStockItemType.IntervalsCalendar字段指定的日历上的日历间隔数
 |
| `material` | material | `VARCHAR` | PK / NOT NULL | Reference |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：时序覆盖天数（Range of Coverage）安全库存动态计算
* **因果流向**：`ipc_io_safety_stock_range_of_coverage` 定义了采用覆盖天数（ROC）策略的物料在不同计划阶段的库存防线。与 DOS 策略类似，ROC 安全库存会根据未来预测销量的变动进行动态涨落。
* **计算算法编排**：
  1. 获取目标覆盖天数 $Cover\_Days$。
  2. 动态向前积分累加：对每一天 $t$，根据未来预测需求 $Demand(\tau)$ 计算安全水位：
     $$ SS_{ROC}(t) = \sum_{\tau = t + 1}^{t + Cover\_Days} Demand(\tau) $$
  3. 下限拦截：若未来总需求为零，自动将安全库存拦截在 `min_units_limit` 上。

###### 2. 物理内存结构设计 (C++ DOD Layout)
ROC 参数在内存中连续存储，便于数轴消纳引擎在只读并行计算中高频访问：
```cpp
// 对应 ipc_io_safety_stock_range_of_coverage 的内存物理结构
struct RangeOfCoverageRecord {
    uint32_t part_id;                 // 物料 ID
    uint32_t site_id;                 // 站点 ID
    int target_cover_days;            // 目标覆盖天数 (对应 range_of_coverage 换算天数)
    double min_units_limit;           // 兜底最小库存套数
};
```

###### 3. 边界与异常处理
* **预测中断处的库存雪崩防护**：若计划展期末端由于预测数据未维护导致 $Demand$ 陡降为零，ROC 算出的安全库存会随之雪崩。引擎在计算时，若发现 $t + Cover\_Days$ 超出了预测数据的最大日期，自动采用最后一期的日平均需求作为常数外推补充，平抑库存波动。