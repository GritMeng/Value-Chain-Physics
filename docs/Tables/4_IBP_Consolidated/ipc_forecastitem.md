---
table_name: "ipc_forecastitem"
alias: "forecastitem"
module: "4_IBP_Consolidated"
cpp_struct: "ForecastItemRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_forecastitem` (forecastitem)

> **业务说明**: 预测

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `forecast_item` | forecast_item | `VARCHAR(10)` | PK / NOT NULL | 预测编号 |
| `usage` | usage | `VARCHAR(1)` | Nullable | - |
| `level` | level | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：需求预测实体归口管理与维表校验
* **因果流向**：`ipc_forecastitem` 是统计预测与需求管理的基本维表。定义了哪些物料-客户-渠道组合构成了一个独立的预测项目（Forecast Item）。预测引擎在运行时，以此表为维表驱动主循环，对每个有效的 Forecast Item 逐一运行平滑模型。
* **维表检验编排**：
  - 物料有效性比对：校验物料是否处于 `ipc_part` 中。
  - 初始化预测容器：为每个 Forecast Item 预分配内存空间，准备时序向量。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存中，预测项目作为高频调用的核心实体，与对应的物料 ID 和客户 ID 直接绑定：
```cpp
// 对应 ipc_forecastitem 的 C++ 内存物理对齐结构体
struct ForecastItemRecord {
    uint32_t forecast_item_id;   // 预测项目 ID (对应 id)
    uint32_t part_id;            // 关联的物料 ID (对应 part)
    uint32_t customer_id;        // 关联的客户 ID (对应 customer)
    uint16_t channel_id;         // 渠道编码 ID
};
```

###### 3. 边界与异常处理
* **垃圾记录动态剪枝**：若某物料已在主物料表中被标记为 `DELETED`，引擎在加载 `ipc_forecastitem` 时会自动过滤清除已失效的预测组合，释放内存。