---
table_name: "ipc_control_class"
alias: "control_class"
module: "7_Control_Data_Model_CDM"
cpp_struct: "ControlClassRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_control_class` (control_class)

> **业务说明**: 控制组

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `description` | description | `VARCHAR` | Nullable | 描述 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：控制组策略分流与计划模式锁定
* **因果流向**：`ipc_control_class` 定义了全局计划策略控制的分流组。Planners 通过将特定的物料-工厂指派给不同的 control_class，来为它们选择不同的求解器配置分支（如：允许替代货源分配、或强制刚性 LBL-MRP 不允许替代）。
* **物理内存结构**：
```cpp
// 对应 ipc_control_class 的内存结构
struct ControlClassRecord {
    uint32_t control_class_id;    // 控制组 ID (对应 control_class)
    bool enforce_rigid_mrp;       // 是否强行禁止物料替代
    bool allow_alternate_sourcing;// 是否允许物流替代货源
};
```