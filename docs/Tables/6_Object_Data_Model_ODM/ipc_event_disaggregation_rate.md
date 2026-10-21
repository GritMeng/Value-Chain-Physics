---
table_name: "ipc_event_disaggregation_rate"
alias: "event_disaggregation_rate"
module: "6_Object_Data_Model_ODM"
cpp_struct: "EventDisaggregationRateRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_event_disaggregation_rate` (event_disaggregation_rate)

> **业务说明**: 该表用于“事件管理”。它提供了当对预测应用基于事件的数量调整时所执行的计算的可见性.
要应用数量调整到预测项目，受影响的预测细节报告在首先使用为事件阶段指定的日历存储ForecastDetails表。
例如，如果事件阶段为每个月增加一定数量的预测数量，那么受影响期间的预测详细信息将使用月份日历进行分类。
结果报告在EventDisaggregationRate表.
对于在应用事件阶段之前没有预测数量的预测项目，也可以将记录添加到EventDisaggregationRate表中。
只受单价调整的预测项目不包括在EventDisaggregationRate表。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `date` | date | `VARCHAR` | Nullable | - |
| `event_phase` | event_phase | `VARCHAR` | PK / NOT NULL | Reference |
| `event_phase_header` | event_phase_header | `VARCHAR` | Nullable | Reference |
| `qty` | qty | `VARCHAR` | Nullable | 数量 (Quantity) |
| `weight` | weight | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：大促预测增量 Top-Down 细分分解
* **因果流向**：当在产品系列或大区层面录入了一个大促预测增量时，`ipc_event_disaggregation_rate` 规定了如何自顶向下将这部分增量分摊至底层的 SKU、工厂和具体大客户。
* **分解速率核算**：
  - 读取大促增量 $Q_{event}$。
  - 获取子物料 $i$ 在当前活动下的分解速率 $Disaggregation\_Factor_i$。
  - 计算子物料分摊增量：
     $$ Q_i(t) = Q_{event}(t) \times Disaggregation\_Factor_i $$

###### 2. 物理内存结构设计 (C++ DOD Layout)
大促分解速率在内存中与促销事件绑定，存储为紧凑的 SoA 对齐映射结构：
```cpp
// 对应 ipc_event_disaggregation_rate 的 C++ DOD 结构体
struct EventDisaggregationRateRecord {
    uint32_t event_id;                // 关联的促销事件 ID
    uint32_t part_id;                 // 底层物料 SKU ID (对应 part)
    uint32_t customer_id;             // 目标客户 ID
    double disaggregation_factor;     // 分解分摊比例 (对应 rate)
};
```

###### 3. 边界与异常处理
* **未定义速率时的销售比例兜底**：若某物料未配置大促专属分解速率，引擎会自动提取该 SKU 过去 90 天的常规实际销售占比（`ipc_his_demand_actual`）进行代数归一化，作为其默认分解速率，平滑过度需求分解。