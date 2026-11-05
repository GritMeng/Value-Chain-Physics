---
table_name: "ipc_resource_capacity_override"
alias: "capacity_override"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcResourceCapacityOverrideRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_resource_capacity_override` (capacity_override)

> **业务说明**: CapacityOverride表用于按一周中的特定日期定义工作中心的容量(如工作时间、效率、资源数量)。如果一个工作中心在该表中定义了一周中某一天的一条或多条有效记录，那么该工作日的记录将覆盖该工作中心的标准Capacity记录。对于工作中心在此表中没有有效记录的天数，则从capacity表中获取容量(假设工作中心在此表中定义了记录)。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `work_center` | work_center | `VARCHAR(10)` | PK / NOT NULL | Reference Table: WorkCenter |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | 生效日期 |
| `efficiency` | efficiency | `DECIMAL(18,2)` | Nullable | 用于缩放此工作中心的操作设置和运行时间的因子。换句话说，就是衡量你的假设产出和实际产出。当与Utilization因子相乘时，得到的值就是使用因子。这用于将运行时间和设置时间从“标准小时”转换为实际经过的小时。 |
| `hours_per_day` | hours_per_day | `DECIMAL(18,2)` | Nullable | 每天工作时长 |
| `number_of_resources` | number_of_resources | `DECIMAL(18,2)` | Nullable | 此工作中心可用的资源数量。
使用此值调整工作中心的可用容量，该值必须是非负的。
如果有多个可用资源，并且允许在给定操作上进行批分割(由
Operation.MaxResource决定)，那么操作将比只使用一个资源时更快地完成。 |
| `utilization` | utilization | `DECIMAL(18,2)` | Nullable | 该因素用于扩展此工作中心的操作设置和运行时间，以允许一天中的非生产时段(例如，休息时间)。当与
效率因子，结果值是使用因子。这用于将运行时间和设置时间从
“标准小时”到实际经过的小时 |
| `day_of_week` | day_of_week | `VARCHAR` | Nullable | 本记录中工作中心容量详细信息适用的星期几.
Monday to Sunday。
如果有必要，每个工作中心可以在一周的某一天有多个有效记录。例如，每个记录可能代表一天中不同的班次，具有自己的工作时间、资源、效率和利用率。
把这些记录放在一起，就能得出工作中心当天的总容量。如果以这种方式设置容量，请确保为每个记录提供一个Index值。
 |
| `index` | index | `VARCHAR` | Nullable | 区分每条记录。
此记录的Index。如果有多个记录具有相同的effecveindate, DayOfWeek和
，则应为每条记录分配不同的Index。例如，该指数可能指的是一天中的特定变化。
注意，如果将相同的Index分配给具有相同的effecveindate、DayOfWeek和
工作中心，这些记录中只有一个将有助于工作中心的能力(其他将被忽略) |
| `working_hour` | working_hour | `VARCHAR` | Nullable | - |
| `descriotion` | descriotion | `VARCHAR` | PK / NOT NULL | 描述 |


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
// 对应 ipc_resource_capacity_override 的 C++ DOD 物理对齐结构体
struct IpcResourceCapacityOverrideRecord {
    std::string work_center; // work_center 字符串 (Reference Table: WorkCenter)
    int eff_start_date = -1; // eff_start_date 相对计划天数 (生效日期)
    double efficiency = 0.0; // efficiency 数量/金额精度值 (用于缩放此工作中心的操作设置和运行时间的因子。换句话说，就是衡量你的假设产出和实际产出。当与Utilization因子相乘时，得到的值就是使用因子。这用于将运行时间和设置时间从“标准小时”转换为实际经过的小时。)
    double hours_per_day = 0.0; // hours_per_day 数量/金额精度值 (每天工作时长)
    std::string working_hour; // working_hour 字符串 (-)
    std::string descriotion; // descriotion 字符串 (描述)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。