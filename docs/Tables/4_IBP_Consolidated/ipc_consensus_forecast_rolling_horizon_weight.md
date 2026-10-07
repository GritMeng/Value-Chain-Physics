---
table_name: "ipc_consensus_forecast_rolling_horizon_weight"
alias: "consensus_forecast_rolling_horizon_weight"
module: "4_IBP_Consolidated"
cpp_struct: "RollingHorizonWeight"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_consensus_forecast_rolling_horizon_weight` (consensus_forecast_rolling_horizon_weight)

> **业务说明**: 当使用滚动预测权重来创建共识需求计划时，ConsensusForecastRollingHorizonWeight表报告了在给定范围内应用于预测类别的权重。结果基于HistoricalDemandCategoryRollingWeight和HistoricalDemandHeaderRollingWeight表。它还报告了预测权重值的来源

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `source` | source | `VARCHAR` | PK / NOT NULL | 表示用于计算共识预测的权重值的来源。
有效值为:
HisDemandHeaderRollingWeight
HisDemandHeader
HistoricalDemandCategory
HistoricalDemandCategoryRollingWeight |
| `header` | header | `VARCHAR` | PK / NOT NULL | - |
| `weight` | weight | `VARCHAR` | Nullable | - |
| `horization` | horization | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多时限延迟加权共识算法
* **因果流向**：随着预测时间向远期推移，不同需求源的准确率会发生置换（例如：下月交期的预测以销售实际为准，6个月后的预测以统计趋势为准）。`ipc_consensus_forecast_rolling_horizon_weight` 规定了这些不同前置时间（Lag）下的渠道加权矩阵。
* **加权计算编排**：
  1. 计算当前需求日期与 RunDate 的天数差（Lag）。
  2. 根据 Lag 匹配对应的权重行：
     $$ Weight_{combined} = w_{sales}(Lag) \times Q_{sales} + w_{stat}(Lag) \times Q_{stat} $$

###### 2. 物理内存结构设计 (C++ DOD Layout)
加权矩阵在内存中以密集二维表形式存储，支持 $O(1)$ 的前置时间索引定位：
```cpp
// 单个延迟时段的权重配置
struct RollingHorizonWeight {
    int horizon_lag_buckets;          // 前置 Lag 周期数
    double sales_weight;              // 销售渠道权重
    double marketing_weight;          // 市场渠道权重
    double statistical_weight;        // 统计渠道权重
};

// 对应 ipc_consensus_forecast_rolling_horizon_weight 的内存结构
struct ConsensusForecastRollingHorizonWeightRecord {
    uint32_t horizon_id;
    std::vector<RollingHorizonWeight> lags; // 各 Lag 时段的权重数组
};
```

###### 3. 边界与异常处理
* **权重不平衡自校准**：若某时段各渠道权重之和不等于 1.0，引擎会自动除以其和进行自归一化，避免计算出的需求总量非理性萎缩或膨胀。