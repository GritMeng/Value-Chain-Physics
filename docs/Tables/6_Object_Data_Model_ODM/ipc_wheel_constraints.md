---
table_name: "ipc_wheel_constraints"
alias: "wheel_constraints"
module: "6_Object_Data_Model_ODM"
cpp_struct: "WheelConstraintRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_wheel_constraints` (wheel_constraints)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `part` | part | `INTEGER` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `site` | site | `VARCHAR` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `source` | source | `VARCHAR` | PK / NOT NULL | - |
| `constraint` | constraint | `VARCHAR` | PK / NOT NULL | Reference:Constraint |
| `min_factor` | min_factor | `VARCHAR` | Nullable | 最小消耗约束的数量 |
| `wheel` | wheel | `VARCHAR` | Nullable | Reference:Wheel |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：轮式排产（Cyclic Scheduling）维度换型时间矩阵控制
* **因果流向**：在流程制造与半导体生产中，工作中心换型准备时间严重依赖于前后生产物料的特征属性（如：从白色墨水换到黑色墨水仅需 5 分钟，而从黑色墨水洗枪换到白色墨水需要 180 分钟）。`ipc_wheel_constraints` 存储了这些顺序依赖型换型约束（Sequence-Dependent Setup Constraints），直接决定了排产时工单的排序决策。
* **轮式排产排程逻辑**：
  1. 读取属性维度：提取当前已排产工单的特征 ID 与即将排产的工单特征 ID。
  2. 检索换型时长：在矩阵中查询 `transition_setup_hours` 并将其累加到工单的准备耗时（Setup Duration）中。
  3. 洗枪转产成本核算：累加 `transition_cleaning_cost` 到财务损失核算。排程算法（启发式或遗传算法）倾向于通过对工单进行重排以最小化总换型时间与洗枪成本。

###### 2. 物理内存结构设计 (C++ DOD Layout)
为了支持 $O(1)$ 的高速 Setup 时间查询，换型约束表在内存中被编译为扁平一维的密集转产时间矩阵 `SetupTransitionMatrix`，消除了复杂的 Key 拼接查找：
```cpp
// 对应 ipc_wheel_constraints 的内存 DOD 结构体
struct WheelConstraintRecord {
    uint32_t work_center_id;          // 工作中心 ID
    uint32_t from_product_group_id;   // 起始产品组特征 ID
    uint32_t to_product_group_id;     // 目标产品组特征 ID
    double transition_setup_hours;    // 换型耗时 (小时) (对应 transition_setup_hours)
    double transition_cleaning_cost;  // 清枪财务成本
};
```

###### 3. 边界与异常处理
* **未定义换型默认惩罚**：如果两个产品组之间的转产关系在矩阵中未定义，系统默认使用保守的最大换型时间（Setup Penalty）进行约束拦截，迫使求解器避免将其排在一起，并输出配置报错。