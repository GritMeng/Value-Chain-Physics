---
table_name: "ipc_event"
alias: "event"
module: "7_Control_Data_Model_CDM"
cpp_struct: "EventRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_event` (event)

> **业务说明**: Event表用于对相关的事件阶段进行分组，事件管理算法将使用这些阶段进行预测调整。此表用于参考，以帮助组织和跟踪事件阶段。EventPhase表中的两个事件阶段可以具有相同的名称和调整类型，只要它们属于不同的事件

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `creation_date` | creation_date | `VARCHAR` | Nullable | 创建此记录的日期 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |
| `event` | event | `VARCHAR` | Nullable | 唯一标识 |
| `type` | type | `VARCHAR` | Nullable | Reference : EventType |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：S&OP 促销大促活动需求膨胀核算
* **因果流向**：`ipc_event` 是因果预测的主索引，定义了具体的促销活动（如：双11线上促销、新品预售发布会）的时段和预计带来的需求放大倍率。S&OP 引擎读取该表，将统计预测的基准值乘以该大促膨胀因子，直接拉动远期的采购与产能。
* **需求叠加算法**：
  1. 检索事件日期：锁定大促的开始与结束天数 $[start\_day, end\_day]$。
  2. 时序数量相乘：对该期间的每一天 $t$，根据常规统计预测量 $Qty_{stat}$ 计算大促拉动值：
     $$ Qty_{event}(t) = Qty_{stat}(t) \times base\_quantity\_multiplier $$
  3. 传递给 `ipc_event_consensus_forecast_detail` 进行财务预测折算。

###### 2. 物理内存结构设计 (C++ DOD Layout)
活动事件在内存中按时间段索引存储在连续数组中，便于时序爆破引擎执行快速遍历：
```cpp
// 对应 ipc_event 的 C++ DOD 物理数据结构
struct EventRecord {
    uint32_t event_id;               // 促销事件 ID (对应 event)
    int start_day;                   // 事件相对开始天数 (对应 start_date)
    int end_day;                     // 事件相对失效天数
    double base_quantity_multiplier; // 销量放大乘数因子 (对应 multiplier)
    uint32_t event_type_id;          // 事件类别 ID
};
```

###### 3. 边界与异常处理
* **大促活动重叠与乘数级联防护**：若同一物料在相同日期内叠加了两个不同的活动（如“全网大促”与“大客户专属优惠”），引擎计算乘数时采用“累加”而非“累乘”逻辑（即 $1 + (M_1-1) + (M_2-1)$），防止需求在双重活动下呈指数级离性膨胀，造成供应链严重过量备料。