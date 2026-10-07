---
table_name: "ipc_node_tans"
alias: "node_tans"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcNodeTansRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_node_tans` (node_tans)

> **业务说明**: 任意Node对应的上层

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `ans_material_source` | ans_material_source | `VARCHAR` | Nullable | - |
| `material_source` | material_source | `VARCHAR` | PK / NOT NULL | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：网络运输边参数表。定义供应链网络图（Supply Chain Graph）中，任意两个物理节点（Node）之间的运输提前期、运费率及日历限制。
* **计算逻辑编排**：
  1. 运费与交期平衡：定义运输成本 $C_{trans}$ 与运输周期 $T_{trans}$。CTP 求解器在寻找最优履约路径时，通过最短路径算法（Dijkstra）求解多目标优化问题，寻找费用与交期的 Pareto 最优解；2. 运力约束判定：在运输高峰日历，运力扣减 `ipc_carrier` 额度，超载则触发建议排程延期。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_node_tans 的 C++ DOD 物理对齐结构体
struct IpcNodeTansRecord {
    std::string ans_material_source; // ans_material_source 字符串 (-)
    std::string material_source; // material_source 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。