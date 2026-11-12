---
table_name: "ipc_production_frequency"
alias: "production_frequency"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcProductionFrequencyRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_production_frequency` (production_frequency)

> **业务说明**: 定义Wheel中物料制造的频率。递归式关联一个物料，也可以关联一个物料的segment。 频率和所对应的Wheel的日历是相同。
通过向Frequency添加偏移量，可以在构建物料时偏移生产周期使他们错开。
例如，如果一个Group中有两个零件（A和B），他们之间的转换成本很高，我们可以在同一个Wheel上生产它们，但不是在该Wheel的相同周期内生产。
例如
Part Recurrence Offset Production Pattern
A      2                   0       Cycles 1, 3, 5, and so forth
B      2                   1       Cycles 2, 4, 6, and so forth


| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `frequency` | frequency | `VARCHAR` | PK / NOT NULL | - |
| `cycle` | cycle | `VARCHAR` | Nullable | 定义周期：
例如，如果零件A为3的Frequency，它可以在Wheel的每三个周期中生产。A可以在第1、4、7、10 Cycle 中构建，以此类推。如果一个零件Frequency为1，则该零件可以在Wheel的每个周期中生产 |
| `offset` | offset | `VARCHAR` | Nullable | 一个零件的生产周期数被抵消。
偏移量为0时，开始生产周期1中的零件。偏移量为1时，开始生产周期2的零件偏移量为2时，开始生产周期3的零件，以此类推 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：主数据层级拓扑映射与沙盘数据逻辑隔离算法
* **因果流向**：系统主数据及控制配置表。
* **计算逻辑编排**：
  1. 数据加载与主数据校验：引擎启动时从物理数据库中读取该表记录，通过 Hash 映射机制将字符串主键转译为 $O(1)$ 的内存索引 ID，建立缓存友好的 SoA 内存块；
2. 时序对齐与时空平移：结合计划日历（CalendarRecord），对记录中的日期字段进行天级偏移（Day Offset）计算，在时间数轴上与独立需求或在手水位对齐；
3. 供需消纳与抵扣：依据 Composite Priority 优先级位权或 FIFO 滑动窗口，对数量字段进行原子扣减或比例分摊计算：\n      $$ Qty_{{effective}}(t) = \\max\\left(0, Qty_{{request}}(t) - Qty_{{allocated}}(t)\\right) $$\n
4. 指标同步与回写：计算结果暂存在线程局部的事务栈中，确认齐套后批量落库，并级联更新上层财务账本或控制塔指标看板。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_production_frequency 的 C++ DOD 物理对齐结构体
struct IpcProductionFrequencyRecord {
    std::string frequency; // frequency 字符串 (-)
    std::string descriotion; // descriotion 字符串 (描述)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。