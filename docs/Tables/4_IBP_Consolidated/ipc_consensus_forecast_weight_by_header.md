---
table_name: "ipc_consensus_forecast_weight_by_header"
alias: "consensus_forecast_weight_by_header"
module: "4_IBP_Consolidated"
cpp_struct: "ConsensusForecastWeightByHeaderRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_consensus_forecast_weight_by_header` (consensus_forecast_weight_by_header)

> **业务说明**: ConsensusForecastWeightByHeader表按报头报告结果的一致预测权重。报告的值将用于ConsensusForecast的计算。结果基于使用以下输入表之一设置的ConsensusForecastWeight:
HistoricalDemandCategory
HistoricalDemandCategoryRollingWeight
HistoricalDemandHeader
HistoricalDemandHeaderRollingWeight
HistoricalDemandHeaderTimePhasedAttributes

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `date` | date | `VARCHAR` | PK / NOT NULL | - |
| `header` | header | `VARCHAR` | PK / NOT NULL | - |
| `weight` | weight | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：大客户专用需求源加权覆盖
* **因果流向**：`ipc_consensus_forecast_weight_by_header` 允许针对特定产品系列或特定大客户（Header）设置专属的权重合并逻辑，覆盖通用的滚动权重矩阵。
* **分配逻辑**：
  - 优先级检测：当对 SKU-Customer 进行共识计算时，若此表中存在客户专属记录，则忽略 `rolling_horizon_weight` 的通用配置，强制套用此表的权重行进行计算。

###### 2. 物理内存结构设计 (C++ DOD Layout)
大客户专属权重在内存中以只读扁平哈希表存储，支持高速的专属分支过滤：
```cpp
// 对应 ipc_consensus_forecast_weight_by_header 的内存物理结构
struct ConsensusForecastWeightByHeaderRecord {
    uint32_t consensus_header_id;     // 共识需求头 ID
    uint32_t source_forecast_header_id;// 来源预测流头 ID
    double specific_weight;           // 覆盖采用的专属权重值 (对应 weight)
};
```

###### 3. 边界与异常处理
* **空引用的安全回退**：若关联的预测源已被删除，引擎会自动回退到通用滚动权重配置，避免产生空值异常。