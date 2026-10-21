---
table_name: "ipc_logistic_location"
alias: "logistic_location"
module: "6_Object_Data_Model_ODM"
cpp_struct: "LogisticLocationRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_logistic_location` (logistic_location)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `location` | location | `VARCHAR(10)` | Nullable | 具体的物理库位编码 |
| `type` | type | `VARCHAR(10)` | Nullable | Airport
Customs
Logistics
Por |
| `address` | address | `VARCHAR` | Nullable | 地址, 可用来在ControlTower里和map集成 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多级分销网络网点位置校验与控制塔（Control Tower）地理映射
* **因果流向**：`ipc_logistic_location` 定义了企业物流网络中的各节点属性，包括工厂、区域配送中心（RDC）、中央仓库（CDC）、港口、机场以及客户收货点。物流调度引擎及控制塔看板读取该表，计算网点间的最短运输路径与中转时效，并在前端进行 GIS 地图渲染与航线可视呈现。
* **位置校验编排**：
  1. 类型校验（Type Validation）：根据 `type`（Airport, Customs, CDC, Port...），对进入该网点的物料执行清关时间偏移、机场安检时效偏移、港口堆存装卸周期等不同置信度的提前期扣减。
  2. 关税与政策检查：对于跨越 Customs 类型网点的运输，引擎自动关联对应的清关税费与合规文件前置期，将清关周期级联累加至物流转移订单（Logistics Stock Transfer Order）的总 Lead Time 中。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 引擎中，物流网点表被扁平化存储，以逻辑索引取代字符串，用一维连续数组支持网络流算法的节点路由检索：
```cpp
// 对应 ipc_logistic_location 的 C++ 物理结构体
struct LogisticLocationRecord {
    uint32_t location_id;       // 逻辑库位/网点 ID (对应 location 字符串哈希值)
    uint8_t location_type;       // 网点类型枚举 (0=CDC, 1=Airport, 2=Customs...)
    double longitude;            // 地理经度 (用于控制塔距离距离算)
    double latitude;             // 地理纬度
    double fixed_handling_cost;  // 节点固定装卸成本
    int transit_clearance_days;  // 关口清关时延天数 (针对 Customs)
};
```

###### 3. 边界与异常处理
* **节点关闭熔断机制**：若某网点临时因不可抗力或天气原因关闭，引擎在 `ipc_logistic_location` 状态标志中将其设为 `INACTIVE`。求解器在搜索发货路线（Delivery Route）时，使用 Dijkstra 算法会自动避开该节点，重新计算次优运输路径，并对因此导致的交期延迟进行级联预警。