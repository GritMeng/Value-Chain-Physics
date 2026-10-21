---
table_name: "ipc_work_center_type"
alias: "work_center_type"
module: "6_Object_Data_Model_ODM"
cpp_struct: "IpcWorkCenterTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_work_center_type` (work_center_type)

> **业务说明**: 该表确定了每种工作中心类型的开关和默认容量值

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `control_class` | control_class | `VARCHAR(10)` | PK / NOT NULL | 控制组 Reference Tablle: ControlGroup |
| `work_center_type` | work_center_type | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `capacity_method` | capacity_method | `VARCHAR` | Nullable | 指定如何在每日和每周工作中心负载记录中报告机器和劳动力能力字段。
有效值为:
HoursPerday - 该工作中心每日/每周报告的能力汇总为：HoursPerDay*NumberOfResource*Efficiency*Utilization*Run
Ratio。
DemonstratedCapacity - 该工作中心的每日/每周报告能力汇总将使用
DemonstratedCapacity.
MaxCapacity |
| `operation_rule` | operation_rule | `VARCHAR` | Nullable | Ignore - 对于引用此WorkCenterType的任何工作中心，在Load表上都不会生成关于此工作中心的任何信息.
OutsideOperation - 不产生Load报表，但scheduling会考虑
Scheduling - 完全考虑. |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 描述 |
| `work_day_start_time` | work_day_start_time | `VARCHAR` | Nullable | 确定工作日开始时的开始时间。如果WorkDayStartTime字段在Capacity或CapacityOverride表中未指定。 |
| `wait_time` | wait_time | `DECIMAL(18,2)` | Nullable | 用于调度此工作中心的所有操作的操作后等待时间. < Capacity.WaitTime<WaitTimeOverride  |
| `queue_time` | queue_time | `DECIMAL(18,2)` | Nullable | < Capacity.WaitTime<WaitTimeOverride  |
| `number_of_resources` | number_of_resources | `DECIMAL(18,2)` | Nullable | 可用于调度使用此工作中心的所有操作的并行资源的数量(如果不适用)
记录的能力。该值必须是非负的。当工作中心没有有效能力记录时，使用该值 |
| `machine_setup_ratio` | machine_setup_ratio | `DECIMAL(18,2)` | Nullable | 一个标准小时的安装所需的机器时间的小时数。此因子用于将工作中心设置时间负载转换为机器负载。当工作中心没有有效容量记录时，使用该值。 |
| `machine_run_ratio` | machine_run_ratio | `DECIMAL(18,2)` | Nullable | 一个标准小时的运行时间所需要的机器时间的小时数。此因子用于将工作中心运行时负载转换为机器负载。当工作中心没有有效容量记录时，使用该值 |
| `labor_setup_ratio` | labor_setup_ratio | `DECIMAL(18,2)` | Nullable | - |
| `labor_run_ratio` | labor_run_ratio | `DECIMAL(18,2)` | Nullable | - |
| `hours_per_day` | hours_per_day | `DECIMAL(18,2)` | Nullable | - |
| `report_limit` | report_limit | `VARCHAR` | Nullable | 用于为某些基于工作中心的计算定义报告期间的结束buckets数量。表示若干
PlanningCalendars.TimeUnits后的时间单位，用于报告以下内容:
计算基于Capacity的指标时，最后一次使用的日期会包括用于计算的活动数据。建议将此值设置为所有工作中心类型的通用值，以避免在不同场景中出现不同的值导致的指标结果混乱。
在WorkCenterCapacity表中报告有效工作中心容量值的最晚可能日期
在各种计算表中显示容量而不显示负载的记录。并不是说记录总是创建到工作中心上有Load的最后日期。此外，在此限制之前，将为每个后续日期创建显示Capacity(和no Load)的记录。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：工作中心类别定义表。定义工作中心为流水线（FlowLine）、离散机台（Discrete）、瓶颈测试机（TestEquipment），控制产能有限匹配时的算法分流。
* **计算逻辑编排**：
  1. 能力排程逻辑选择：若是 FlowLine，采用基于批次流（Batching Flow）的排班，计算清洗与换型间隔；若是 Discrete，进入多约束排程（MCDM）算法，对机器、模具、人员工时实施联合求解。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_work_center_type 的 C++ DOD 物理对齐结构体
struct IpcWorkCenterTypeRecord {
    std::string control_class; // control_class 字符串 (控制组 Reference Tablle: ControlGroup)
    std::string work_center_type; // work_center_type 字符串 (唯一标识)
    std::string descriotion; // descriotion 字符串 (描述)
    std::string work_day_start_time; // work_day_start_time 字符串 (确定工作日开始时的开始时间。如果WorkDayStartTime字段在Capacity或CapacityOverride表中未指定。)
    double wait_time = 0.0; // wait_time 数量/金额精度值 (用于调度此工作中心的所有操作的操作后等待时间. < Capacity.WaitTime<WaitTimeOverride)
    double queue_time = 0.0; // queue_time 数量/金额精度值 (< Capacity.WaitTime<WaitTimeOverride)
    double machine_setup_ratio = 0.0; // machine_setup_ratio 数量/金额精度值 (一个标准小时的安装所需的机器时间的小时数。此因子用于将工作中心设置时间负载转换为机器负载。当工作中心没有有效容量记录时，使用该值。)
    double machine_run_ratio = 0.0; // machine_run_ratio 数量/金额精度值 (一个标准小时的运行时间所需要的机器时间的小时数。此因子用于将工作中心运行时负载转换为机器负载。当工作中心没有有效容量记录时，使用该值)
    double labor_setup_ratio = 0.0; // labor_setup_ratio 数量/金额精度值 (-)
    double labor_run_ratio = 0.0; // labor_run_ratio 数量/金额精度值 (-)
    double hours_per_day = 0.0; // hours_per_day 数量/金额精度值 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。