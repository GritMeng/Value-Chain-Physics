---
table_name: "ipc_forecast_causal_factor"
alias: "causal_factor"
module: "4_IBP_Consolidated"
cpp_struct: "CausalFactorRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecast_causal_factor` (causal_factor)

> **业务说明**: 此表存储了在生成统计预测之前用于调整历史数据的数据错误或异常需求事件的因果因素。它引用部分客户和因果因素所应用的历史实际(通过HistoricalDemandHeader引用)，以及因果因素所关联的类别(通过CausalFactorCategory引用)。因果因素的其他详细信息，如调整数量和日期，存储在引用此表的CausalFactorDetail表中. 这个是业务统计输入的.

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `category` | category | `VARCHAR` | PK / NOT NULL | Causal类别 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：因果因素剔除与基准需求重构
* **因果流向**：`ipc_forecast_causal_factor` 存储了对历史实际销量产生非随机波动的外部事件（如促销、价格调整、突发天气）。统计预测引擎在进行模型训练前，必须根据该表将历史实际出货量中的因果波动剔除，还原出纯净的基准需求（Baseline Demand），否则会造成未来常规预测值严重失真。
* **因果剥离算法**：
  - 数据加载：读取 `ipc_his_demand_actual` 历史需求序列。
  - 剥离计算：在对应事件日期，扣除对应的因果因子调整量：
     $$ History_{cleaned}(t) = History_{raw}(t) - Causal\_Impact(t) $$
  - 将干净的 $History_{cleaned}$ 作为 Holt-Winters 或 ARIMA 的输入序列进行模型训练，生成常规预测。

###### 2. 物理内存结构设计 (C++ DOD Layout)
因果因素元数据在内存中采用关联结构表示，支持预测引擎在历史序列清洗循环中进行 O(1) 的事件查询：
```cpp
// 对应 ipc_forecast_causal_factor 的 C++ DOD 物理数据结构
struct CausalFactorRecord {
    uint32_t causal_id;          // 因果因素 ID (对应 causal_factor)
    uint32_t category_id;        // 类别 ID (对应 category)
    uint32_t history_header_id;  // 关联的历史实际需求头 ID
};
```

###### 3. 边界与异常处理
* **因果过度清洗防护**：若配置的因果调整量过大，导致剥离计算后历史实际销量出现负数，引擎会自动将该天的 $History_{cleaned}$ 设为 0，防止负值销量输入预测模型导致平滑系数发散。