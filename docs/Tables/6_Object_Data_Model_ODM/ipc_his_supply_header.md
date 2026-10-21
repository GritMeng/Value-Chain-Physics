---
table_name: "ipc_his_supply_header"
alias: "his_supply_header"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcHisSupplyHeaderRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_his_supply_header` (his_supply_header)

> **业务说明**: 此表标识每个唯一的部件、供应商和历史供应类别组合。该表中的每个条目通常与多个历史供应系列或历史供应实际记录相关联

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `category` | category | `VARCHAR` | PK / NOT NULL | 唯一标识 |
| `material_supplier` | material_supplier | `VARCHAR` | PK / NOT NULL | Part.Name, PartSupplier.Part.Site,
PartSupplier.Supplier
Part.Name, PartSupplier.Part.Site,
PartSupplier.Supplier
 |
| `lead_time_calendar` | lead_time_calendar | `VARCHAR` | Nullable | 用于表示与此抬头下的历史供应实际情况相关的已知交货时间的日历。
也就是说，如果HistoricalSupplyActual。前置时间被填充，它被假定在这个日历的间隔中。LeadTime字段用于假定历史交货时间可变性的安全库存计算，但在没有提供OrderDate的情况下，因此无法计算历史交货时间。如果这里没有指定，那么默认everyday。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：历史供应头表。标识特定 SKU 与供应商、站点的关系，绑定对应的交期日历，是历史供应明细的索引入口。
* **计算逻辑编排**：
  1. 日历映射：读取 lead_time_calendar 确定该供应商在计算提前期偏差时的有效工作日；2. 统计汇聚：根据物料供应商联合索引，聚合实际到料历史，生成各供应商交付延迟的概率分布模型（Probability Density Function）。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_his_supply_header 的 C++ DOD 物理对齐结构体
struct IpcHisSupplyHeaderRecord {
    std::string category; // category 字符串 (唯一标识)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。