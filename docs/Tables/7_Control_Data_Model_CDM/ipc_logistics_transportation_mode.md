---
table_name: "ipc_logistics_transportation_mode"
alias: "transportation_mode"
module: "7_Control_Data_Model_CDM"
cpp_struct: "TransportationModeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_logistics_transportation_mode` (transportation_mode)

> **业务说明**: 运输的模式：陆运，海运等

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `transportation_mode` | transportation_mode | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `description` | description | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：时空权衡（Speed-Cost Trade-off）运输模式抉择
* **因果流向**：当跨站点调拨订单或客户交货单在进行 CTP 可行性承诺时，引擎通过 `ipc_logistics_transportation_mode` 评估选择不同运输模式（如航空货运、铁路快运、海运或陆运）时的总提前期与物流成本，在“时效最快”与“成本最低”之间进行多目标决策（MCDM）最优化求解。
* **权衡算法编排**：
  1. 模式读取与速度折算：获取不同运输模式的平均速度及中转处理时间。
  2. 运输成本核算：
     $$ Cost_{transport} = Qty \times (Cost_{fixed} + Cost_{var} \times Distance) $$
  3. 决策求解：在客户订单面临延迟交付惩罚（Penalty Cost）时，引擎评估如果将运输模式从“海运”（慢，便宜）切换为“空运”（快，昂贵），空运溢价（Air Freight Premium）是否小于延迟交期的扣款。若小于，则自动切换运输模式并锁定空运提前期，生成优化后的运输指令。

###### 2. 物理内存结构设计 (C++ DOD Layout)
运输模式是网络路由图的边属性（Edge Attributes）。在 C++ 内存中，其各项标量因子以稠密数组存储，以支持 Dijkstra 算法在多层网络路径上的超高速松弛迭代：
```cpp
// 对应 ipc_logistics_transportation_mode 的 C++ DOD 结构
struct TransportationModeRecord {
    uint8_t mode_id;                 // 运输模式逻辑编码 (对应 transportation_mode)
    double average_speed_km_h;       // 平均运输时速 (用于 LeadTime 自动推算)
    double base_cost_per_kg;         // 基础运费系数 (对应固定成本)
    double variable_cost_per_km_kg;  // 动态里程运费系数 (对应可变成本)
    double co2_emission_factor;      // 碳排放系数 (碳中和约束核算)
};
```

###### 3. 边界与异常处理
* **气候与季节性提前期拉伸**：在冬季或台风季，海运和空运的实际提前期会发生伸缩。引擎支持在运行参数中设置拉伸乘数（Stretching Factor），动态调整 `average_speed_km_h` 速度，防止在排产计算中高估运输能力。