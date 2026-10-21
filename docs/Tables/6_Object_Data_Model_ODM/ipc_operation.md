---
table_name: "ipc_operation"
alias: "operation"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_operation` (operation)

> **业务说明**: 零件生产过程中所需要的每一项操作/工艺。这些记录描述了操作的顺序、操作的持续时间以及在操作处理期间工作中心可用容量的任何差异(或覆盖)。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `operation` | operation | `VARCHAR(10)` | PK / NOT NULL | 唯一标识 |
| `descriotion` | descriotion | `VARCHAR` | Nullable | 操作的描述 |
| `sequence` | sequence | `INTEGER` | Nullable | 对工艺路线此操作所属的特定序列。一个操作应该属于工艺路线的标准序列，或者属于它的任何可选并行序列.
Reference Table:OperationSequence |
| `work_center` | work_center | `VARCHAR(10)` | Nullable | 工作中心
Reference Table: WorkCenter |
| `operation_type` | operation_type | `VARCHAR(10)` | Nullable | 操作类型
Reference Table: OperationType |
| `wait_time` | wait_time | `DECIMAL(18,2)` | Nullable | 等待运行结束的时间. Type中定义时间单位 |
| `transit_time` | transit_time | `DECIMAL(18,2)` | Nullable | 操作之间的转移时间。在Type中指定是此Operation的前序还是后序。 |
| `teardown_time` | teardown_time | `DECIMAL(18,2)` | Nullable | 清楚物料，以及把此操作恢复到可以接新订单的时间。 |
| `setup_time` | setup_time | `DECIMAL(18,2)` | Nullable | 设置机器的之间 |
| `run_time` | run_time | `DECIMAL(18,2)` | Nullable | 每单位所用的运行时间。 |
| `queue_time` | queue_time | `DECIMAL(18,2)` | Nullable | 排队时间.
在调度此操作时，使用此值作为队列时间，而不是工作中心指定的队列时间。
如果该字段为负值，则使用工作中心值。
这是完成操作设置之前等待的小时数，只有当它大于或等于零时才使用。 |
| `dimen_grp` | dimen_grp | `VARCHAR(10)` | Nullable | Reference Table: dimension_group |
| `batch_qty` | batch_qty | `DECIMAL(18,2)` | Nullable | 此工序完成，下一个工序可以开始的数量 |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | - |
| `eff_end_date` | eff_end_date | `DATE` | Nullable | - |
| `max_resources` | max_resources | `DECIMAL(18,2)` | Nullable | 可并发执行此操作的最大工作中心资源数。如果可以在资源之间并行操作，则操作应该更快完成。
该值与工作中心的可用资源数量中较小者用于确定应用于操作的实际资源数量。 |
| `queue_time_override` | queue_time_override | `DECIMAL(18,2)` | Nullable | - |
| `wait_time_override` | wait_time_override | `DECIMAL(18,2)` | Nullable | 可选地，用于指定在工作中心完成操作运行后等待时间的小时数。
如果该字段为负值，则使用适用于工作中心的Capacity记录上指定的WaitTime值。否则，如果此值大于或等于零，它将覆盖WaitTime值。 |
| `routing` | routing | `VARCHAR(10)` | PK / NOT NULL | 工艺路线。关联routing表 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：工艺工序明细表。定义特定工艺路线上每一个工步（Operation）的 setup_time, run_time 及工作中心绑定关系，是有限产能排配的物理内核。
* **计算逻辑编排**：
  1. 工步负荷爆炸：根据计划工单数量与单件工时（run_time）计算工序负荷工时；2. 机器约束绑定：根据绑定的 work_center 索引，将计算工时在开工期 $S$ 扣减对应工作中心的 capacity 水位；3. 换型优化：若该工步存在 setups 换型，读取 setups 参数加入计划开工天偏移：\n      $$ Start\_Day_{{active}} = Start\_Day_{{base}} - setup\_time / HoursPerDay $$\n

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_operation 的 C++ DOD 物理对齐结构体
struct alignas(64) IpcOperationRecord {
    std::string operation; // operation 字符串 (唯一标识)
    std::string descriotion; // descriotion 字符串 (操作的描述)
    double wait_time = 0.0; // wait_time 数量/金额精度值 (等待运行结束的时间. Type中定义时间单位)
    double transit_time = 0.0; // transit_time 数量/金额精度值 (操作之间的转移时间。在Type中指定是此Operation的前序还是后序。)
    double teardown_time = 0.0; // teardown_time 数量/金额精度值 (清楚物料，以及把此操作恢复到可以接新订单的时间。)
    double setup_time = 0.0; // setup_time 数量/金额精度值 (设置机器的之间)
    double run_time = 0.0; // run_time 数量/金额精度值 (每单位所用的运行时间。)
    std::string dimen_grp; // dimen_grp 字符串 (Reference Table: dimension_group)
    double batch_qty = 0.0; // batch_qty 数量/金额精度值 (此工序完成，下一个工序可以开始的数量)
    int eff_start_date = -1; // eff_start_date 相对计划天数 (-)
    int eff_end_date = -1; // eff_end_date 相对计划天数 (-)
    double queue_time_override = 0.0; // queue_time_override 数量/金额精度值 (-)
    uint32_t routing; // routing 逻辑ID/映射 (工艺路线。关联routing表)
    uint32_t site; // site 逻辑ID/映射 (工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant))
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。