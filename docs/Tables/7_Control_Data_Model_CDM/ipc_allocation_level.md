---
table_name: "ipc_allocation_level"
alias: "allocation_level"
module: "7_Control_Data_Model_CDM"
cpp_struct: "AllocationLevelRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_allocation_level` (allocation_level)

> **业务说明**: 从战略战术角度，需求的分配层级或者说优先级

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `allocation_level` | allocation_level | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `level` | level | `VARCHAR(10)` | Nullable | High
Mid
Low |
| `allocation_rule` | allocation_rule | `VARCHAR(10)` | Nullable | FIFS - 按优先级
FAIR - 按比例分
EQUAL - 等数量分配
 |
| `allocation_calendar` | allocation_calendar | `VARCHAR(10)` | Nullable | 时间单位 |
| `allocation_intervals` | allocation_intervals | `DECIMAL(18,2)` | Nullable | 多少个时间单位为周期 |
| `allocaiton_window` | allocaiton_window | `DECIMAL(18,2)` | Nullable | allocation_calendar*allocation_intervals*allocation_window 为分配horizon。 在这个horizon内遵循allocation_rule的规则，之后遵循FIFS |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供需 Pegging 颗粒度与控制层级路由
* **因果流向**：`ipc_allocation_level` 规定了计划引擎在执行供需消纳（Pegging）和订单承诺时，逻辑所处的控制层级。它决定了资源是在具体 SKU-Site 层面独占，还是在产品系列或全球站点网格（Global Network）层面共享。
* **路由控制编排**：
  - 读取本表对应的粒度参数。
  - 若为 `SKU_SITE` 层级：消纳计算完全独立。
  - 若为 `PRODUCT_FAMILY` 级别：跨物料间可以执行库存代用与 fair-share 跨站点调拨。

###### 2. 物理内存结构设计 (C++ DOD Layout)
分配粒度作为求解器全局编译标志，以位字段（Bit-field）直接对齐：
```cpp
// 分配粒度枚举
enum class AllocationGranularity : uint8_t {
    SKU_SITE_CUSTOMER = 0,
    SKU_SITE = 1,
    PRODUCT_FAMILY_SITE = 2
};

// 对应 ipc_allocation_level 的内存物理结构
struct AllocationLevelRecord {
    uint32_t allocation_level_id;     // 分配层级 ID
    AllocationGranularity granularity;// 分配粒度枚举 (对应 allocation_level)
};
```

###### 3. 边界与异常处理
* **高粒度策略自动向下切分**：若全局设为 `PRODUCT_FAMILY` 级别共享，但在运行时检测到大客户签署了排他性独占协议，引擎会自动在内存中对该订单关联的 SKU 进行局部“切片降级”，强制实行 `SKU_SITE_CUSTOMER` 独占分配，确保合同合规。