---
table_name: "ipc_rtl_sc"
alias: "rtl_sc"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcRtlScRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_rtl_sc` (rtl_sc)

> **业务说明**: 根节点到叶节点的供应链网络模型

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `dis_part` | dis_part | `VARCHAR(40)` | Nullable | 下游物料或者是assemble 物料 |
| `source_part` | source_part | `VARCHAR(40)` | Nullable | 上游物料或者是BOM ITEM的物料 |
| `driver_part` | driver_part | `VARCHAR(40)` | Nullable | 根物料 |
| `level` | level | `DECIMAL(18,2)` | Nullable | 基于根的供应链网络中层级 |
| `driver_per_qty` | driver_per_qty | `DECIMAL(18,2)` | Nullable | 此节点对应没单位根节点转化的数量 |
| `driver_lt` | driver_lt | `DECIMAL(18,2)` | Nullable | 此节点到根节点的累计提前期 |
| `per_qty` | per_qty | `DECIMAL(18,2)` | Nullable | 没单位下游物料需要多少单位上游物料 |
| `lt` | lt | `DECIMAL(18,2)` | Nullable | 上游物料转化成下游物料的提前期 |
| `eff_unit_price` | eff_unit_price | `DOUBLE` | Nullable | 转化的单位成本 |
| `eff_end_date` | eff_end_date | `DATE` | Nullable | 上游物料的可用日期 |
| `dimension_grp` | dimension_grp | `VARCHAR(10)` | Nullable | 消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配 |
| `bom_id` | bom_id | `VARCHAR(10)` | Nullable | bomid |
| `driver_site` | driver_site | `VARCHAR(8)` | Nullable | 根供应源 |
| `dis_site` | dis_site | `VARCHAR(8)` | Nullable | 下游site |
| `alt_grp` | alt_grp | `VARCHAR(10)` | Nullable | 替代组编码，相同替代组内的组件物料属于可替换物料 |
| `if_leaf` | if_leaf | `BOOLEAN` | Nullable | Y - 该节点为叶节点  N - 该节点不是叶节点 |
| `is_phantom` | is_phantom | `BOOLEAN` | Nullable | 是否为虚拟物料。Y - 虚拟件 (展开子BOM)，N - 实体件 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：实时/近短期补货供应链网络表。定义 IOP 阶段，分钟级/天级调拨的路径与前置期约束。
* **计算逻辑编排**：
  1. 短期寻路：CTP 引擎在进行每日/每小时实时订单拼单决策时，优先检索此表确定近端备料库房；2. 快速运输策略匹配：如果订单具有 Composite Priority 高优先级，系统可自动匹配极速空运路径，缩短提前期但扣减相应的 logistics_premium 成本费率。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_rtl_sc 的 C++ DOD 物理对齐结构体
struct IpcRtlScRecord {
    uint32_t dis_part; // dis_part 逻辑ID/映射 (下游物料或者是assemble 物料)
    uint32_t source_part; // source_part 逻辑ID/映射 (上游物料或者是BOM ITEM的物料)
    uint32_t driver_part; // driver_part 逻辑ID/映射 (根物料)
    double level = 0.0; // level 数量/金额精度值 (基于根的供应链网络中层级)
    double driver_per_qty = 0.0; // driver_per_qty 数量/金额精度值 (此节点对应没单位根节点转化的数量)
    double driver_lt = 0.0; // driver_lt 数量/金额精度值 (此节点到根节点的累计提前期)
    double per_qty = 0.0; // per_qty 数量/金额精度值 (没单位下游物料需要多少单位上游物料)
    double lt = 0.0; // lt 数量/金额精度值 (上游物料转化成下游物料的提前期)
    double eff_unit_price = 0.0; // eff_unit_price 数量/金额精度值 (转化的单位成本)
    int eff_end_date = -1; // eff_end_date 相对计划天数 (上游物料的可用日期)
    std::string dimension_grp; // dimension_grp 字符串 (消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配)
    uint32_t bom_id; // bom_id 逻辑ID/映射 (bomid)
    uint32_t driver_site; // driver_site 逻辑ID/映射 (根供应源)
    uint32_t dis_site; // dis_site 逻辑ID/映射 (下游site)
    std::string alt_grp; // alt_grp 字符串 (替代组编码，相同替代组内的组件物料属于可替换物料)
    bool if_leaf = false; // if_leaf 布尔标志 (Y - 该节点为叶节点  N - 该节点不是叶节点)
    bool is_phantom = false; // is_phantom 布尔标志 (是否为虚拟物料。Y - 虚拟件 (展开子BOM)，N - 实体件)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。