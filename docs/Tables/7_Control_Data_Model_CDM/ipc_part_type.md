---
table_name: "ipc_part_type"
alias: "part_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcPartTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_part_type` (part_type)

> **业务说明**: 包含控制零件处理方式的值。通过适当地设置记录，您可以从ERP源模拟零件类型(也称为源代码)。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `part_type` | part_type | `VARCHAR(10)` | Nullable | 物料状态，唯一标识 |
| `description` | description | `VARCHAR` | Nullable | 物料描述 |
| `fc_calendar` | fc_calendar | `VARCHAR(10)` | Nullable | 预测消耗用的日历。
Reference Table: Calendar |
| `fc_alt` | fc_alt | `VARCHAR(10)` | Nullable | backward_alt - 从订单的DueDate/DataDate（有MaterialType.ConcumpationDate决定）开始向后冲减到前一个Bucket（区间）的开始日期，然后向后冲减到下个Bucket（区间）的结束日期。如此往复。

forwar_alt - 从订单的DueDate/DataDate（有MaterialType.ConcumpationDate决定）开始向前冲减到前一个Bucket（区间）的开始日期，然后向前冲减到下个Bucket（区间）的结束日期。如此往复

Backward - 在订单所在的Bucket（区间）内向后冲减。


Forward - 在订单所在Bucket（区间）内向前冲减。

Normal - 在订单所在的区间向后然后向前。

默认值： Normal |
| `fc_date` | fc_date | `DATE` | Nullable | DataDate - 需求创建日期
RequestDueDate - 需求在仓库准确好的日期
默认值：RequestDueDate(先看DemandOperation的值，再看这个值，以DemandOperation为准） |
| `netting_alt` | netting_alt | `VARCHAR(10)` | Nullable | 为每个物料类型指定一个处理规则，该规则描述用于Netting计算的逻辑。
有效值是:
ignor- 所有计划绝不考虑这个物料。这部分的所有供求和供给记录被净额忽略，仅显示但标识其不处理。因为忽略了所有的供应和需求，没有计划订单或可用的承诺记录生成。
 
MRP- MRP需要考虑的原材料或组件，如果其有BOM, 展开但不做下层可用性检查。
 
MPS - 其预测会被销售订单消耗。可用的承诺信息生成。预测记录(有需求类型的独立需求。OperationRule = '生产预测')不被销售订单消耗，而是对主生产计划部件的有效需求。

family -用于对父部件进行BOM展开到期组件预测并驱动对其组件部件的需求。此配置用于按产品系列进行预测。标识产品族的BOMType为family(其BOM为计划bom，创建它是为了定义预测的部件与其组件之间的关系，并拥有BOMType.family_demand_source = ' Y ')。实际的部件需求在消耗上级的预测。未消耗的预测结果在计划订单和约束被忽略，即只参与Forecast展开到下层，不参与其他计划。

其默认值设置为MRP, 因为参与MRP的原材和部件占绝大多数。
 |
| `ctp_date_rule` | ctp_date_rule | `VARCHAR(10)` | Nullable | Site的订单执行LT整体来看还是精细化分段管理。
Y - 分段管理
N- 取物料上的OELT
C - 启用Constrain |
| `fc_window_rule` | fc_window_rule | `VARCHAR(10)` | Nullable | 指定未消耗的预测如何在Forecast window内如何处理。
有效值是:
Ignor -任何在Window内未消耗的预测总是被净额忽略(供应不是计划来满足它)
Include -在窗口内未消耗的预测依然考虑 |
| `ControlGroup` | control_class | `VARCHAR(10)` | PK / NOT NULL | Reference Table : ControlGroup |
| `ss_rule` | ss_rule | `VARCHAR(10)` | Nullable | Reference Table : ss_rule |
| `priority` | priority | `VARCHAR` | Nullable | 优先级，数值越小越优先 |
| `allocation_level` | allocation_level | `DATE` | Nullable | 为每种物料类型指定如何维护订单承诺，并设置如何将优先级应用MRP Netting (供应计划)和CTP(可用日期计算)。有效值为:

High - 
Mid - 
Low - 


 |
| `excess` | excess | `VARCHAR` | Nullable | 如果一个物料被配置为全局或bom级替代品，此设置可用于指定只有其多余的供应可用于满足其他物料的需求。
在物料定义的剩余的窗口内。在剩余物料（剩余部分）可用作替代品之前，零件上的现有供应将首先用于满足其自身需求。
fence - 如果该物料被配置为bom级替代组件，则其当前供应应首先用于满足其自身的任何需求（无论订单优先级如何）。剩余的或“过剩”的供应然后有资格满足其他组件的需求。
global_fence - 如果该部件被配置为全局替代部件，则应首先使用其当前供应来满足其自身的任何需求.剩余或“过剩”的供应，然后有资格满足来自其他部分的相同优先级的需求，它已被配置为替代品。有了这个设置，计划的订单将只生成以满足零件自身的需求，而不是那些它是替代品的零件的需求（由计划的那些产生的超额）

 |
| `preference` | preference | `VARCHAR(10)` | Nullable | Z – 尽量用优先级高的替换料。
1.	优先级高的On-Hand库存。
2.	在Tolerance 公差范围内预定接收原料件 SR。
3.	原料件的准时、现有计划订单的剩余量。
4.	在优先级高的物料上创建准时的计划订单。
5.	优先级低的库存。
6.	在Tolerance公差范围内预定接收替代件SR。
7.	替代件的按时、现有计划订单的过剩量。
8.	最早计划收货SR，但在组内任何一个的最早计划订单之前可用。
9.	在优先级高的物料上创建准时计划订单。
10.	来自任何一个物料的最迟的现有计划订单。
11.	创建最少延迟的计划订单
N – 尽量用最早的供应满足
1. 优先级高的现有库存。
2. 优先级低的现有库存。
3. 优先级高的预计收货时间在容差区间内。
4. 优先级低的预计到货时间在容差区间内。
5. 优先级高的现有计划订单按时完成后的剩余量。
6. 优先级低的现有计划订单按时完成后的剩余量。
7. 最早预计收货时间超出公差区间（如果在最早计划订单之前可用）。
8. 在优先级高的物料上创建按时计划订单。
9. 在优先级低的物料上创建按时计划订单。
10. 来自最晚存在的计划订单的剩余量。
11. 创建计划订单，且该订单的计划时间最晚。
C – 跟Z类似，但是要用完现有的供应，包括On-Hand, SR, Excess
 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：物料分类定义表。定义物料属于 Finished Goods（成品）、Semi-finished（半成品）、Raw Materials（原材料）或 Phantom（虚拟件），是求解器爆破的分水岭。
* **计算逻辑编排**：
  1. 行为控制逻辑映射：在加载 part 主数据时，若 part_type 为 'Phantom'，系统将 LLC 设置为上层零件的 LLC + 1，并在展开时不调用产能和提前期，直接穿透爆炸；若为 'Raw'，则阻断其向下展开，仅生成采购建议。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_part_type 的 C++ DOD 物理对齐结构体
struct IpcPartTypeRecord {
    uint32_t part_type; // part_type 逻辑ID/映射 (物料状态，唯一标识)
    std::string description; // description 字符串 (物料描述)
    std::string ControlGroup; // ControlGroup 字符串 (Reference Table : ControlGroup)
    std::string ss_rule; // ss_rule 字符串 (Reference Table : ss_rule)
    std::string priority; // priority 字符串 (优先级，数值越小越优先)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。