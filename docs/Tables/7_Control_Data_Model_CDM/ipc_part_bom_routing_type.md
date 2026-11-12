---
table_name: "ipc_part_bom_routing_type"
alias: "part_bom_routing_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcPartBomRoutingTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_part_bom_routing_type` (part_bom_routing_type)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | Nullable | 控制组
Reference Table: ControlGorup |
| `bom_type` | bom_type | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `date_rule` | date_rule | `VARCHAR(10)` | Nullable | fixed_date - 手工或者单据上的固定日期	
cal_due_date - due date	
cal_dock_date - 到门日期
cal_start_date - 动作开始执行日期	
always - 一直可用 |
| `eff_rule` | eff_rule | `VARCHAR(10)` | Nullable | never - 不用
always - 一直用
in_ex - 从开始日期开始直到结束日期之前
ex- 从开始日的下一个日期开始到结束日期	
in - 从开始日期到结束日期	
ex - 不包括结束日期	 |
| `assembly_yield` | assembly_yield | `BOOLEAN` | Nullable | 是否应用Yield.
Y - 考虑父阶yield
N - 不考虑父阶yield |
| `rouding_rule` | rouding_rule | `VARCHAR(10)` | Nullable | up - 下一个整数
down - 上一个整数 
nearest - 四舍五入到整数
none - 不取整
per_qty - 只用BOM中的per_qty, 跟需求数量无关
默认值： None |
| `scrap_rule` | scrap_rule | `VARCHAR(10)` | Nullable | ignore - 忽略
yield_fraction - 从0到1， 1意味着no loss.
yield_percent - 从1到100， 100意味着没有损失
scrap_fraction - 从0到1， 0意味着no loss
scrap_percent - 从1到100， 0意味着no loss
scrap_fixed - 每个单据都是损失一个固定的数量
inflation_fraction - 1/(1+part_source.yield)
inflation_percent - 1/(1+part_source.yield*0.01) |
| `phantom` | phantom | `BOOLEAN` | Nullable | y - phantom
n- non-phantom
defual - y |
| `explore_negative` | explore_negative | `BOOLEAN` | Nullable | Y -  负数参与计算
N -  负数不参与计算
注意， Co-product/By-product将会忽略此设置，认为都是Y. 此处的Y用于测试物料场景等 |
| `assembly_type` | assembly_type | `VARCHAR(10)` | Nullable | normal - 常规处理
co_product - 一种可以通过与装配（主产品）产品一同制造并规划出来的副产品，这是因为它们具有相同的结构、组件以及工艺上的相似性。例如，这种副产品可能就是主要产品的低等级版本。网状结构；网状物对于具有副产品关系的部件，其 CTP 计算也会同时进行。因此，
双向的副产品关系是被支持的	
by_product - 一种由装配（主产品）产品生产过程产生并伴随其存在的联产品。例如，一种联产品可能是在给定产品结构中生产一个或多个组件时产生的某种化学物质。对于联产品部件的净额计算和 CTP 计算是在其所有物料清单组件都已规划完毕之后进行的。因此，双向联产品关系不被支持。	 |
| `ratio_rule` | ratio_rule | `VARCHAR(10)` | Nullable | ignore - 忽略
fraction - 0到1
percen - 0到100
 |
| `use_lt_offset` | use_lt_offset | `BOOLEAN` | Nullable | Y - 所提供的提前期值将用于所有提前期计算中。将此值设为“Y”后，即可将“bom_item.lt_offset”字段作为除“part”和“part_source”等基于提前期的元素（如固定提前期、可变提前期和安全提前期）之外的另一个因素，用于计算产品提前期和开始日期。
N - 不考量 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：物料BOM路由绑定类别表。定义物料与工艺路线绑定的属性（如量产路线、小样路线、试制路线），控制在求解器中的选择优先级。
* **计算逻辑编排**：
  1. 默认路径路由：工单生成时，系统按 routing_type 默认过滤，批量生产工单默认过滤 'Production'，研发工单默认过滤 'Prototype'；2. 降级匹配：若主生产路径无产能，系统可降级选择 alternative 路线。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_part_bom_routing_type 的 C++ DOD 物理对齐结构体
struct IpcPartBomRoutingTypeRecord {
    uint32_t bom_type; // bom_type 逻辑ID/映射 (唯一标识)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。