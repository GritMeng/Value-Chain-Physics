---
table_name: "ipc_constraint"
alias: "constraint"
module: "6_Object_Data_Model_ODM"
cpp_struct: "ConstraintRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_constraint` (constraint)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `constraint` | constraint | `VARCHAR(10)` | PK / NOT NULL | 唯一标识符
Part number
Supplier + Part number
Bottleneck resource name
Assembly line or production area
Supplier + part family
 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `description` | description | `VARCHAR` | Nullable | 描述 |
| `constraint_type` | constraint_type | `VARCHAR(10)` | Nullable | 约束类型，指定约束被使用的规则
Reference Table: ConstrainType |
| `cumulative_max` | cumulative_max | `DECIMAL(18,2)` | Nullable | 当超过这个值时约束已经不再可用，例如合同的可用量 |
| `calendar` | calendar | `VARCHAR(10)` | Nullable | 日期 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：全局瓶颈能力校验与硬边界控制
* **因果流向**：`ipc_constraint` 表定义了全局制造瓶颈资源（如关键测试机台、SMT 线体或特定大客户的采购合同上限）。
* **控制链编排**：
  1. 天级能力检验：在 CTP 预占工段产能时，引擎读取当前约束的可用周期日历 `calendar` 与最大允许累计量 `cumulative_max`（合同额度）。
  2. 刚性拦截：若某天或全周期的累计负荷突破硬性上限，系统实施刚性阻断，触发交期顺延（Backlog）或转产，确保不出现伪可行排产计划。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存中，约束被翻译为连续的 `ConstraintRecord` 结构体：

```cpp
// 关联 ipc_constraint 表的 C++ DOD 数据结构
struct ConstraintRecord {
    uint32_t constraint_id;              // 约束逻辑 ID (Offset)
    std::string constraint_code;         // 约束物理名称 (对应 constraint)
    std::string constraint_type;         // 约束类别
    std::vector<double> rates;           // 时序天级可用上限
    std::vector<double> allocated_rates; // 时序天级已占用负荷
};
```