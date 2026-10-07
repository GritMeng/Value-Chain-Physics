---
table_name: "ipc_production_cycle"
alias: "production_cycle"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcProductionCycleRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_production_cycle` (production_cycle)

> **业务说明**: 关于Wheel的周期的报告，包括周期开始和结束日期、周期中使用的约束以及周期中可能发生的约束可用性变化

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `con_constraint` | con_constraint | `VARCHAR(10)` | Nullable | Reference Table: Constrain |
| `site` | site | `VARCHAR` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `con_constraint2` | con_constraint2 | `VARCHAR(10)` | Nullable | Reference Table: Constrain |
| `part` | part | `INTEGER` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `con_site` | con_site | `VARCHAR` | Nullable | - |
| `source` | source | `VARCHAR` | Nullable | - |
| `constraint` | constraint | `VARCHAR` | Nullable | Reference:Constraint |
| `constraint_available` | constraint_available | `DECIMAL(18,2)` | Nullable | - |
| `constraint_used` | constraint_used | `DECIMAL(18,2)` | Nullable | - |
| `end_date` | end_date | `VARCHAR` | Nullable | - |
| `start_date` | start_date | `VARCHAR` | Nullable | - |
| `cycle_number` | cycle_number | `VARCHAR` | Nullable | Wheel的Cycle编号。例如，周期数为3表示该周期是生产轮中的第三个周期。 |
| `wheel` | wheel | `VARCHAR` | Nullable | Reference:Wheel |


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
// 对应 ipc_production_cycle 的 C++ DOD 物理对齐结构体
struct IpcProductionCycleRecord {
    std::string con_constraint; // con_constraint 字符串 (Reference Table: Constrain)
    uint32_t site; // site 逻辑ID/映射 (工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant))
    std::string con_constraint2; // con_constraint2 字符串 (Reference Table: Constrain)
    int part = 0; // part 整型数值 (物料唯一编码 (Part Code))
    uint32_t con_site; // con_site 逻辑ID/映射 (-)
    std::string source; // source 字符串 (-)
    std::string constraint; // constraint 字符串 (Reference:Constraint)
    double constraint_available = 0.0; // constraint_available 数量/金额精度值 (-)
    double constraint_used = 0.0; // constraint_used 数量/金额精度值 (-)
    std::string end_date; // end_date 字符串 (-)
    std::string start_date; // start_date 字符串 (-)
    std::string cycle_number; // cycle_number 字符串 (Wheel的Cycle编号。例如，周期数为3表示该周期是生产轮中的第三个周期。)
    std::string wheel; // wheel 字符串 (Reference:Wheel)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。