---
table_name: "ipc_produciton_grp"
alias: "produciton_grp"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcProducitonGrpRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_produciton_grp` (produciton_grp)

> **业务说明**: ProductionGroup标识Wheel生产的一组物料。 为每个Group分配了用于主要和次要changeover的约束数量，以及组可以使用的最大约束数量。此外，当组与具有相同零件序列的其他组一起生产时，每个组被分配一个优先级来管理。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `production_group` | production_group | `VARCHAR(10)` | PK / NOT NULL | - |
| `description` | description | `VARCHAR` | Nullable | - |
| `major_cleanup` | major_cleanup | `DECIMAL(18,2)` | PK / NOT NULL | 从一个Group的Material转移到另一个Group的物料所需的约束量。
模拟在Group的零件清理共享资源/生产线/机器所需的时间。以约束单位测量。 |
| `major_setup` | major_setup | `DECIMAL(18,2)` | Nullable | 从一个Group的Material转移到当前Group的物料需要准备约束量。
 |
| `max_consumption` | max_consumption | `DECIMAL(18,2)` | Nullable | 在Wheel中此Group最大的消耗量,数值为百分比.
例如，如果将此值设置为0.5，则有
如果一个Cycle中有500个约束可用，则此Group在该周期中只能使用不超过250个约束。最大约束消耗包括用于Group中每个部件的约束、为组设置和清理所需的约束，以及用于在组中每个部件之间进行转换的约束。
除MajorCleanUp外，当达到此限制时不能在此周期中计划与Group相关的物料.为生产组分配给MajorCleanUp的约束量可能会导致该最大值超过其限制，并允许计划继续进行.
 |
| `minor_change_over` | minor_change_over | `DECIMAL(18,2)` | Nullable | Group中Material转换所需的转换约束数 |
| `priority` | priority | `VARCHAR` | Nullable | 优先级，数值越小越优先 |
| `site` | site | `VARCHAR` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |


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
// 对应 ipc_produciton_grp 的 C++ DOD 物理对齐结构体
struct IpcProducitonGrpRecord {
    std::string production_group; // production_group 字符串 (-)
    std::string description; // description 字符串 (-)
    double minor_change_over = 0.0; // minor_change_over 数量/金额精度值 (Group中Material转换所需的转换约束数)
    std::string priority; // priority 字符串 (优先级，数值越小越优先)
    uint32_t site; // site 逻辑ID/映射 (工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant))
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。