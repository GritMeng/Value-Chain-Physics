---
table_name: "ipc_io_safety_stock_time_phased_bounds"
alias: "safety_stock_time_phased_bounds"
module: "5_IO_Safety_Stock"
cpp_struct: "SafetyStockTimePhasedBoundsRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_io_safety_stock_time_phased_bounds` (safety_stock_time_phased_bounds)

> **业务说明**: 此表支持库存计划工作簿。它包含零件的最小和最大安全库存界限以及这些界限开始适用的日期。当使用安全库存界限时，RapidResponse不建议超出指定界限的安全库存数量.
安全库存界限是根据供应天数或数量来规定的。该表可以在同一记录(四个字段)中保存供应天数和数量的最大值和最小值，但一次只能使用一组(供应天数或数量)。SafetyStockItemType设置控制是使用数量限制、供应天数限制，还是两者都不使用。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `date` | date | `VARCHAR` | PK / NOT NULL | 边界应用的日期 |
| `maximum_days_of_supply` | maximum_days_of_supply | `DECIMAL(18,2)` | Nullable | - |
| `minimum_days_of_supply` | minimum_days_of_supply | `DECIMAL(18,2)` | Nullable | - |
| `maximum_qty` | maximum_qty | `DECIMAL(18,2)` | Nullable | - |
| `minimum_qty` | minimum_qty | `DECIMAL(18,2)` | Nullable | - |
| `material` | material | `VARCHAR` | PK / NOT NULL | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：时序安全库存防爆仓红线控制
* **因果流向**：`ipc_io_safety_stock_time_phased_bounds` 定义了计划期内各天或各阶段的安全库存硬性防波堤红线（Bounds）。它用于拦截 MEIO 优化过程中产生过低（导致缺料风险）或过高（导致爆仓积压）的安全库存推荐值。
* **界限拦截算法编排**：
  1. 绝对数量拦截：在 MEIO 计算出推荐安全库存 $SS_{recom}$ 后，进行上限与下限校对：
     $$ SS_{final}(t) = \max\left( min\_units\_limit(t), \min\left( max\_units\_limit(t), SS_{recom}(t) \right) \right) $$
  2. 供应天数转换：若界限以天数定义，引擎读取未来预测值并动态换算为件数限制后，再执行拦截。

###### 2. 物理内存结构设计 (C++ DOD Layout)
界限控制数据通常在 MRP 生成补货工单前被高频读取。在内存中，它被编译为沿时间轴分布的边界向量，以提高判定效率：
```cpp
// 对应 ipc_io_safety_stock_time_phased_bounds 的 C++ 内存结构
struct SafetyStockTimePhasedBoundsRecord {
    uint32_t part_id;          // 物料ID
    uint32_t site_id;          // 站点ID
    int day_bucket;            // 相对计划相对天数
    double min_units_limit;    // 绝对数量下限
    double max_units_limit;    // 绝对数量上限
    double min_days_limit;     // 供应天数下限
    double max_days_limit;     // 供应天数上限
};
```

###### 3. 边界与异常处理
* **上下限倒置容错**：若配置人员误将 $min\_units\_limit$ 设为大于 $max\_units\_limit$，引擎在初始化时会自动将上限修改为与下限相等，确保逻辑通路不会产生负数可用区间导致 MPS 溢出。