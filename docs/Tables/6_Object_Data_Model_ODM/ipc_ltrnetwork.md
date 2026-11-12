---
table_name: "ipc_ltrnetwork"
alias: "ltrnetwork"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcLtrnetworkRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_ltrnetwork` (ltrnetwork)

> **业务说明**: 根节点到叶节点的供应链网络模型II

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `dis_material_source` | dis_material_source | `VARCHAR` | Nullable | - |
| `material_source` | material_source | `VARCHAR` | Nullable | - |
| `driver_part_source` | driver_part_source | `VARCHAR` | Nullable | - |
| `level` | level | `VARCHAR` | Nullable | - |
| `driver_per_qty` | driver_per_qty | `VARCHAR` | Nullable | - |
| `driver_lt` | driver_lt | `VARCHAR` | Nullable | - |
| `per_qty` | per_qty | `VARCHAR` | Nullable | - |
| `lt` | lt | `VARCHAR` | Nullable | - |
| `eff_start_date` | eff_start_date | `VARCHAR` | Nullable | - |
| `eff_end_date` | eff_end_date | `VARCHAR` | Nullable | - |
| `dimension_grp` | dimension_grp | `VARCHAR` | Nullable | 消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配 |
| `bomid` | bomid | `VARCHAR` | Nullable | - |
| `leaf_material_source` | leaf_material_source | `VARCHAR` | Nullable | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：长周期补货网络节点拓扑表。定义物料流在不同分销中心（DC）之间的层级拓扑关系，构建网络图。
* **计算逻辑编排**：
  1. 拓扑排序：求解器读取此表，利用广度优先搜索（BFS）或拓扑排序确定各 DC 的 LLC，确保 MRP 需求自上而下顺畅流动；2. 链条环路校验：阻断双向环路调拨，确保网络是一个有向无环图（DAG）。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_ltrnetwork 的 C++ DOD 物理对齐结构体
struct IpcLtrnetworkRecord {
    std::string dis_material_source; // dis_material_source 字符串 (-)
    std::string material_source; // material_source 字符串 (-)
    uint32_t driver_part_source; // driver_part_source 逻辑ID/映射 (-)
    std::string level; // level 字符串 (-)
    std::string driver_per_qty; // driver_per_qty 字符串 (-)
    std::string driver_lt; // driver_lt 字符串 (-)
    std::string per_qty; // per_qty 字符串 (-)
    std::string lt; // lt 字符串 (-)
    std::string eff_start_date; // eff_start_date 字符串 (-)
    std::string eff_end_date; // eff_end_date 字符串 (-)
    std::string dimension_grp; // dimension_grp 字符串 (消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配)
    uint32_t bomid; // bomid 逻辑ID/映射 (-)
    std::string leaf_material_source; // leaf_material_source 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。