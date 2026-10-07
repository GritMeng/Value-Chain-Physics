---
table_name: "ipc_source_type"
alias: "source_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcSourceTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_source_type` (source_type)

> **业务说明**: 用于定义每个潜在供应源的特性（例如，自制、外购和转移）

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制集合 |
| `cost_rule` | cost_rule | `VARCHAR(10)` | Nullable | unit_cost	- 将“part_source.eff_unit_cost”设置为一个固定值，该值等于导入到“PartSource.UnitCost”字段中的值，或者如果“UnitCost”为 0 时则等于“Part.StdUnitCost”。如果这两个字段的值均不大于 0，则“PartSource.EffUnitCost”的值为 0。如果当 PartSource.UnitCost 大于 0 时，将 PartSource.EffUnitCost 设置为 UOMConversion（PartSource.UnitCost）的值。否则，如果部分。如果标准单位成本大于 0，则将“零件来源.有效单位成本”设置为“零件.标准单位成本”。否则，“零件来源.有效单位成本”设为 0
part_labor_oh_cost	-  |
| `supply_type` | supply_type | `VARCHAR(10)` | Nullable | 对于计划订单，它规定了如何控制订单级别的配置设置（例如重新安排的逻辑以及该订单是“制造”、“采购”，“集团内交易”,还是“转移”）。然而“part_type”和“part_source_type”可能已经进行了命名，以反映“自制”、“外购”或“转移”这三种情况。而“供应类型”则是定义“自制”、“外购”,"公司内交易"和“转移”逻辑的值，而“supply_type”则是这些值之间的关联。 |
| `source_type` | source_type | `VARCHAR(10)` | PK / NOT NULL | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：供应来源类型定义表。定义物料的补货渠道类型为 Make（自制）、Buy（采购）或 Transfer（转移调拨）。
* **计算逻辑编排**：
  1. MRP 展开逻辑分流：在进行多路径 CTP 预占时，读取物料站点的 source_type，指导求解器进入自制（BOM工艺爆炸）还是采购/调拨（物流偏置与前推）的分支逻辑。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_source_type 的 C++ DOD 物理对齐结构体
struct IpcSourceTypeRecord {
    std::string control_class; // control_class 字符串 (控制集合)
    std::string supply_type; // supply_type 字符串 (对于计划订单，它规定了如何控制订单级别的配置设置（例如重新安排的逻辑以及该订单是“制造”、“采购”，“集团内交易”,还是“转移”）。然而“part_type”和“part_source_type”可能已经进行了命名，以反映“自制”、“外购”或“转移”这三种情况。而“供应类型”则是定义“自制”、“外购”,"公司内交易"和“转移”逻辑的值，而“supply_type”则是这些值之间的关联。)
    std::string source_type; // source_type 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。