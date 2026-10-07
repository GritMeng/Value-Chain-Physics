---
table_name: "ipc_carrier"
alias: "carrie"
module: "6_Object_Data_Model_ODM"
cpp_struct: "CarrierRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_carrier` (carrie)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `carrier` | carrier | `VARCHAR(10)` | PK / NOT NULL | 与此承运商关联的唯一标识符 |
| `description` | description | `VARCHAR` | Nullable | 承运商描述 |
| `defual_transportation_mode` | defual_transportation_mode | `VARCHAR(10)` | Nullable | 如果delivery_route中有值，那么用那个值 |
| `transit_calendar` | transit_calendar | `VARCHAR(10)` | Nullable | 为使用本承运人的交货路线定义运输日期的日历参考。因此，该日历决定了如何从到期日期和可用日期分别计算DemandOrderLine的计划收货日期和可用收货日期。如果该引用为Null，则假设使用每日日历。
Reference Table:Calendar |
| `location` | location | `VARCHAR(10)` | Nullable | 具体的物理库位编码 |
| `default_transit_lt` | default_transit_lt | `DECIMAL(18,2)` | Nullable | 对于使用该运输商的客户配送路线，将适用默认的运输时间。如果某个delivery_route记录中的“transit_lt”字段值非负，则将使用该字段值来代替默认值。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：物流承运商资质判定与交期可靠性动态补偿
* **因果流向**：`ipc_carrier` 记录了物流承运商的主数据、运费膨胀因子以及历史准时率。运输路线评估引擎在检索发货计划时，不仅计算承运商的运费成本，还会根据其历史交付准时率（Reliability Rating）动态调整运输提前期的安全缓冲（Buffer LT）。
* **交期补偿算法**：
  1. 读取基准在途提前期 $LT_{base}$。
  2. 动态补偿：计算经过可靠性风险调整后的最终提前期：
     $$ LT_{final} = LT_{base} \times \left( 1 + \delta \times (1.0 - reliability\_rating) \right) $$
     其中 $\delta$ 为风险敏感系数。

###### 2. 物理内存结构设计 (C++ DOD Layout)
承运商参数作为物流图边的权重因子，连续对齐存放以支持 Dijkstra 的高速运行：
```cpp
// 对应 ipc_carrier 的 C++ DOD 结构
struct CarrierRecord {
    uint32_t carrier_id;              // 承运商 ID (对应 carrier)
    double transit_cost_multiplier;   // 运费价格系数乘数
    double reliability_rating;        // 准时可靠率 (对应准时率，0.0 - 1.0)
};
```

###### 3. 边界与异常处理
* **承运商黑名单锁定**：若承运商可靠率跌破 $70\%$，引擎自动触发“降级警告”，在路由搜索时将其运输优先级降至最低，迫使系统改用其他更可靠但价格稍高的物流渠道。