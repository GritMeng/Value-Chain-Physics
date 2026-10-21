---
table_name: "ipc_consensus_forecast_rolling_horizon"
alias: "consensus_forecast_rolling_horizon"
module: "4_IBP_Consolidated"
cpp_struct: "ConsensusForecastRollingHorizonRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_consensus_forecast_rolling_horizon` (consensus_forecast_rolling_horizon)

> **业务说明**: ConsensusForecastRollingHorizon表存储了用于定义在创建共识需求计划时将滚动预测权重应用到单个预测类别的顺序和持续时间的记录

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `sequence` | sequence | `INTEGER` | PK / NOT NULL | 一个在确定生成共识预测时的持续时间的值，将weight应用于Category或者Header |
| `duration` | duration | `INTEGER` | PK / NOT NULL | 时间跨度 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：滚动时效预测周期与决策截断
* **因果流向**：`ipc_consensus_forecast_rolling_horizon` 规定了滚动需求计划的时效周期（Horizon）。它定义了随着时间滚动，共识预测引入各渠道预测的先后次序和总跨度，防范远期数据不确定性污染近期排产。
* **计划期滑动编排**：
  - 每天运行引擎时，根据 RunDate 重新计算 Horizon 窗口，将处于 `duration` 之外的远期预测自动裁剪，不进入 MRP 计算主干。

###### 2. 物理内存结构设计 (C++ DOD Layout)
滚动周期参数作为只读元数据，在内存中扁平化存储：
```cpp
// 对应 ipc_consensus_forecast_rolling_horizon 的内存对齐物理结构
struct ConsensusForecastRollingHorizonRecord {
    uint32_t horizon_id;         // 滚动时效ID (对应 consensus_forecast_rolling_horizon)
    int sequence;                // 阶段顺序号
    int duration_days;           // 持续天数
};
```

###### 3. 边界与异常处理
* **跨周期时段截断**：若计划展期小于滚动周期总和，引擎会自动在展期末端进行强行切断，忽略溢出段，确保内存不越界。