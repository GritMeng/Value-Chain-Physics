---
table_name: "ipc_hierarchy_product"
alias: "product_hierarchy"
module: "6_Object_Data_Model_ODM"
cpp_struct: "ProductHierarchyNode"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_hierarchy_product` (product_hierarchy)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `product` | product | `VARCHAR(40)` | Nullable | - |
| `part_desc` | part_desc | `VARCHAR` | Nullable | - |
| `parent_product` | parent_product | `VARCHAR(40)` | Nullable | - |
| `parent_desc` | parent_desc | `VARCHAR` | Nullable | - |
| `ratio` | ratio | `VARCHAR` | Nullable | 产品层级的比率 |
| `per_qty` | per_qty | `VARCHAR` | Nullable | 每parent所需要的product 数量 |
| `hierarchy_type` | hierarchy_type | `VARCHAR` | Nullable | 层级类型  type.ratio_tule 表示比率所用的规则 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：物料多级物料层次（Product Hierarchy）聚合树构建与分解
* **因果流向**：`ipc_hierarchy_product` 建立了产品层次结构树（从具体 SKU 到子分类、再到大分类、最终到产品族）。它是 S&OP 进行“自上而下分解（Top-Down）”和“自下而上滚动聚合（Bottom-Up Rollup）”的骨架逻辑。
* **树形爆破与聚合算法**：
  1. 预测滚动聚合（Bottom-Up）：
     $$ Q_{parent}(t) = \sum_{child \in Children} Q_{child}(t) $$
  2. 自上而下分解（Top-Down）：大区经理录入大类预测后，系统沿着树形节点关系向下分解，读取各子节点的比率，逐级分摊到 SKU-Site 级预测。

###### 2. 物理内存结构设计 (C++ DOD Layout)
为避免在多级树状结构上反复跳转指针（Pointer Chasing）导致高速缓存（Cache）失效，产品层次树在内存中采用**扁平亲子有向图数组**表示，通过扁平邻接矩阵快速回溯：
```cpp
// 对应 ipc_hierarchy_product 的 C++ 内存物理对齐结构体
struct ProductHierarchyNode {
    uint32_t part_id;            // 物料 ID (对应 part)
    uint32_t parent_node_id;     // 父分类节点 ID (对应 parent)
    uint8_t hierarchy_level;     // 树的层级深度 (0=SKU, 1=SubFamily, 2=ProductFamily...)
};
```

###### 3. 边界与异常处理
* **层级环路硬拦截（Cycle Detection）**：如果数据维护人员误操作将 A 的父级设为 B，B 的父级设为 A，会造成递归聚合计算发生栈溢出。层次预编译器在系统加载时执行循环扫描，一旦发现拓扑环路，立刻刚性熔断报错并停止引擎加载。