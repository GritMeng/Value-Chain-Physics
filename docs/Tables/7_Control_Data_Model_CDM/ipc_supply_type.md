---
table_name: "ipc_supply_type"
alias: "supply_type"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcSupplyTypeRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_supply_type` (supply_type)

> **业务说明**: 用于定义在“supply_order”表中使用的类型，以及处理与“supply_order”记录相关的预定收货的规则。此外，该表还用于“source_type”记录中，以定义处理计划订单时的特征

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `description` | description | `VARCHAR` | Nullable | 供给类型描述 |
| `source_class` | source_class | `VARCHAR(10)` | Nullable | 为每种供应类型指定了一个来源类别。“来源类别 supply_class”这一值与“BOM”相结合，用于控制相关需求（分配）的生成。
值如下：
Make - 将查看组装的BillOfMaterial记录，以确定需要哪一组组件部件来创建该部件的供应。通常，这种类型的供给表示Site本身为该部分构建供应的情况，并且与工作订单和装备操作相关联，在这些操作中，几个不同的组件被组装在一起，形成一个新的组装。

Buy - IPC不会对其下层再产生任何进相关需求。期望采购订单被创建并与管理自己计划的供应商进行沟通。

Transfer-表示将从另一个物料（物料号可以不同）和Site提供该部件的供应。不像Make来源，它不转移到多个组件或考虑BillOfMaterial记录。相反，在消费站点提供的部件只是在供应站点创建对该部件的需求。因此，预期是，如果消费站点对该部件有需求，那么它将从供应站点获得或获取它所需要的部件。


 |
| `ControlGroup` | control_class | `VARCHAR(10)` | PK / NOT NULL | Reference Table : control_class |
| `assignment_demand_type` | assignment_demand_type | `VARCHAR(10)` | Nullable | 其独立需求对应的DemandType
Reference Table : DemandType |
| `assignment_forecast_consumption` | assignment_forecast_consumption | `BOOLEAN` | Nullable | 确定其由上层需求驱动后是否参与ForecastConsuption. 既可以被展开的是预测也可以展开的是实际需求。
Y - 参与
N - 不参与 |
| `operation_rule` | operation_rule | `VARCHAR(10)` | Nullable | ExplodedOnly - 供应被忽略，被认为是不可重新调度的。然而，依赖的需求是由这种类型的供应和预定的接收产生的。ReschedDate(用scheduledreceiver . duedate填充)用于展开BOM。例如，修理订单可能需要组件，但是要发送给客户，并且不能与其他供应品混在一起。约束，如果适用，可能会被消耗。

Ignor—供应在净额和计算表中被忽略。但是，它确实出现在ScheduledReceipt表中。这适用于订单号级别。

In-process - 不可调整。如已经领料生产执行的工单，在途的采购订单。

Reschedulable — 供应可以完全重新调度。意味着这个SR会被删除掉并且重新创建。

RescheduleRecommend - 到货计划是根据需求调整的，IPC会给出更新的要求到货日期.  在数量能满足的情况下使用SR而不产生新的PlannedOrder.

 |
| `sort_policy` | sort_policy | `VARCHAR(10)` | Nullable | 指定Supply的优先级，在Supply assignment过程中Demand先看哪些供给。默认我们以SupplyScheDueDate排序，On-hand为当前日期。值越小优先级越高。
 |
| `supply_type` | supply_type | `VARCHAR(10)` | PK / NOT NULL | - |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：供应资产类型配置表。定义供应的物理本质，包含 OnHand（在库）、InTransit（在途）、WorkInProcess（在制）、Planned（计划中）。
* **计算逻辑编排**：
  1. 消耗优先级分配：规定不同供给类别在 ATP 水位中的扣减权重。CTP 引擎默认按照 OnHand ➔ InTransit ➔ Planned 的刚性降级路径依次扣减，确保最优资产利用率。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_supply_type 的 C++ DOD 物理对齐结构体
struct IpcSupplyTypeRecord {
    std::string description; // description 字符串 (供给类型描述)
    std::string ControlGroup; // ControlGroup 字符串 (Reference Table : control_class)
    std::string supply_type; // supply_type 字符串 (-)
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。