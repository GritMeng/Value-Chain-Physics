---
table_name: "ipc_ans_tnode"
alias: "ans_tnode"
module: "6_Object_Data_Model_ODM"
cpp_struct: "AnsTnodeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_ans_tnode` (ans_tnode)

> **业务说明**: 任意Node对应的下层

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `ans_material_source` | ans_material_source | `VARCHAR` | PK / NOT NULL | - |
| `material_source` | material_source | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多级 BOM 扁平拓扑路径计算与可承诺（CTP）链遍历
* **因果流向**：`ipc_ans_tnode` 记录了从成品到其下属每一阶子件在 BOM 树上的多步逻辑路径与级联乘数。它是计划引擎执行 CTP 递归承诺和低代码（LLC）拓扑编译的加速底座。
* **路径编译算法**：
  1. 树形展平：预编译器扫描 `ipc_bom_item`，利用广度优先搜索（BFS）计算从根成品到叶节点子件的所有物理路径。
  2. 累加提前期与用量：对每一条路径，累加路径上的工艺提前期，并将每一步的 `perqty` 累乘得到 `cumulative_per_qty`。
  3. CTP 高速检索：当销售订单流入需要 CTP 预占时，引擎直接查询此表进行 $O(1)$ 的用量爆破，无需在运行时遍历整棵有向图。

###### 2. 物理内存结构设计 (C++ DOD Layout)
拓扑路径在内存中被编译为紧凑的稠密邻接矩阵或一维连续图节点数组：
```cpp
// 对应 ipc_ans_tnode 的 C++ DOD 数据结构
struct AnsTnodeRecord {
    uint32_t assembly_id;        // 组装件 ID (对应 assemble_part)
    uint32_t component_id;       // 组件 ID (对应 part)
    uint32_t path_length;        // BOM 级联深度层数 (对应 level)
    double cumulative_per_qty;   // 累乘后的单位用量 (对应 per_qty)
};
```

###### 3. 边界与异常处理
* **死循环路径剔除**：若检测到环路，引擎在构建路径时强行抛弃该子路径并产生刚性中断报错，防止 BFS 进入无限循环导致内存耗尽。