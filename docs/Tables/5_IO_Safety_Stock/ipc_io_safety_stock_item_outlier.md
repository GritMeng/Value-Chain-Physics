---
table_name: "ipc_io_safety_stock_item_outlier"
alias: "safety_stock_item_outlier"
module: "5_IO_Safety_Stock"
cpp_struct: "SafetyStockItemOutlierRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_io_safety_stock_item_outlier` (safety_stock_item_outlier)

> **业务说明**: 该表报告与SafetyStockItem相关的历史数据中的异常值相关的详细信息。
类似于ForecastItemParametersOutliers
1. 异常值是根据SafetyStockItem表计算的，结果存储在
SafetyStockItemOutlierResult和SafetyStockItemOutlierResultSummary表。
2. SafetyStockHistoricalSupply中的交货时间和SafetyStockHistoricalDemand中的数量将根据该表中的数据进行更正。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `adjustment` | adjustment | `DECIMAL(18,2)` | Nullable | 离群值调整的数量。它可以是负值 |
| `date` | date | `VARCHAR` | Nullable | 此记录中报告的离群数据所对应的历史日期。这里报告的每个日期都属于项目的Type.IntervalsCalendar。
例如，日期可能标记每周或每月周期的开始。 |
| `operation_rule` | operation_rule | `VARCHAR` | PK / NOT NULL | 指定离群值所属的历史数据系列。
Demand - 历史需求
LeadTime - 历史提前期 |
| `safety_stock_item` | safety_stock_item | `VARCHAR` | PK / NOT NULL | - |
| `usage` | usage | `VARCHAR` | Nullable | Y - 用。 N - 不用 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：需求方差计算中的黑天鹅突发异值拦截与清洗
* **因果流向**：在 MEIO 安全库存计算中，日需求的标准差 $\sigma_D$ 是最敏感的因子。如果历史出货受到“缺货后突发补发”或“一次性大客户爆单”等黑天鹅异值污染，标准差会非理性发散，导致算出的安全库存极高。`ipc_io_safety_stock_item_outlier` 用于拦截这些异常点。
* **清洗算法编排**：
  1. 设定方差清洗时窗（如 90 天滑动窗口）。
  2. 计算销量偏差 Z-score。若 $Z > 3.0$ 判定为 Outlier。
  3. 将该离群点记录写入本表，并在方差计算逻辑中将其销量替换为时窗中位数，从而计算出健康的“常规变异度”，平抑安全库存虚高。

###### 2. 物理内存结构设计 (C++ DOD Layout)
异常明细在内存中连续存放，直接服务于方差并行计算：
```cpp
// 对应 ipc_io_safety_stock_item_outlier 的内存物理结构
struct SafetyStockItemOutlierRecord {
    uint32_t part_id;            // 物料 ID (对应 part)
    uint32_t site_id;            // 站点 ID (对应 site)
    int day_bucket;              // 历史相对天数 (对应 date)
    double outlier_demand_qty;   // 原始异常销量
    double clean_replaced_qty;   // 清洗替换后的销量
};
```

###### 3. 边界与异常处理
* **异常比重过高防护**：若在 90 天内被判定为 Outlier 的天数超过 10%，说明这不是黑天鹅事件，而是市场波动性发生常态化改变。引擎会自动限制清洗上限为前 5% 的最大值，防止过度清洗导致高估服务水平。