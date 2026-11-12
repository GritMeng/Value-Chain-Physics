---
table_name: "ipc_deminsion"
alias: "deminsion"
module: "6_Object_Data_Model_ODM"
cpp_struct: "DimensionRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_deminsion` (deminsion)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `demision` | demision | `VARCHAR(10)` | Nullable | 维度 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 维度的描述 |
| `value` | value | `INTEGER` | Nullable | 维度对应的值，只能为整数数字 |
| `value_description` | value_description | `VARCHAR` | Nullable | 维度对应具体值的描述，例如维度为硬盘值为100, 描述是100G |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多维物料属性特征分级配额控制
* **因果流向**：在半导体和流程工业（如钢铁、化工）中，产出的零部件并不是单一的，而是具有不同的性能维度（如：速度等级、封装批次、碳含量）。`ipc_deminsion` 存储了这些多维属性分类。引擎利用这些维度进行降级消纳（Downgrading Allocation）与级联分配。
* **维度映射编排**：
  - 读取产品实体的 `dimension_val` 属性值。
  - 绑定物料：配合替代 BOM 和 CTP 判定函数，将具有高属性值的物料分配给需要低属性值的客户订单（即：将高性能芯片降级满足中性能芯片的需求），产生降级替代记录 `ipc_swap_record`。

###### 2. 物理内存结构设计 (C++ DOD Layout)
属性维度在内存中作为细化颗粒度标识紧跟物料 ID，采用高精度浮点数存储，支持逻辑判定：
```cpp
// 对应 ipc_deminsion 的 C++ 内存物理结构
struct DimensionRecord {
    uint32_t dimension_id;            // 维度特征 ID (对应 deminsion)
    uint32_t dimension_group_id;      // 维度组 ID
    double quantitative_value;        // 维度的量化特征值 (用于大于/小于等关系算子判定)
};
```

###### 3. 边界与异常处理
* **非标维度非法运算符熔断**：若对于字符型非标准维度（如“红色”）误用了 `LT`（小于）或 `GT`（大于）算子进行分配校验，预占判定引擎会自动在编译拓扑图时拦截，报错熔断并提示更改为 `EQ`（等于）算子，防止运行时崩溃。