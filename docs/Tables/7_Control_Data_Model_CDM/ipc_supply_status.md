---
table_name: "ipc_supply_status"
alias: "supply_status"
module: "7_Control_Data_Model_CDM"
cpp_struct: "IpcSupplyStatusRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_supply_status` (supply_status)

> **业务说明**: 暂无描述

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `reschedule_condition` | reschedule_condition | `VARCHAR(10)` | Nullable | DueDate - 不可调整。
Now - 随意调整，哪怕订单开始日期已经Past。
FreezeDate - 可调整至FreezeDate. 这个算法意味着，跟供应商冻结期，冻结期内的计划不可调整。
RunDate - RunDate和其之后都可以调整。 |
| `rechedule_rule` | rechedule_rule | `VARCHAR(10)` | Nullable | FromSupplyType - 即遵循SupplyType中的设定。
Can't - 日期和数量是固定的，不能重新调度。
Received - 已收货，不能调整。
InTransit - 不可调整。如已经领料生产执行的工单，在途的采购订单。
Reschedulable — 供应可以完全重新调度。意味着这个SR会被删除掉并且重新创建PlannedOrder。
Recommend - 到货计划是根据需求调整的，IPC会给出更新的要求到货日期.  在数量能满足的情况下使用SR而不产生新的PlannedOrder
Built - 已经开始生产，不能调整。
Scheduled - 已经安排了调度，分配给了相应Demand，不可调整。 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：供应链计划引擎分配算法
* **因果流向**：供应单据状态定义表。定义单据状态（如 Draft 草稿、Released 下发、Closed 关闭），控制该订单是否作为可供消纳的供给资产。
* **计算逻辑编排**：
  1. 供应有效性过滤：在 MRP 准备期，引擎过滤仅加载 status 属于 'Released' 或 'In-Transit' 的订单记录；对于 'Draft' 状态的订单予以忽略，不作为现有有效供给。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器中，为了杜绝高频指针跳转引起的 CPU L1/L2 缓存失效（Cache Missing），所有该表数据在初始化时被连续编译入密集一维数组中进行 $O(1)$ 裸金属级寻址：

```cpp
// 对应 ipc_supply_status 的 C++ DOD 物理对齐结构体
struct IpcSupplyStatusRecord {
};
```

###### 3. 边界与异常处理
* **主外键引用失效兜底**：若关联的主物料（Part）或站点（Site）在内存中找不到匹配 ID，引擎自动抛出异常信息，降级为默认备用主数据，防止整个 MRP 展开流程意外中断；
* **数量为负/分母为零防护**：在进行分摊比例或前置时间拉伸计算时，若分母为零，系统自动重置比例为等比例分摊（Fair-Share），若数量出现负数，则截断为零，防止算账结果非理性发散；
* **日期超限自动收缩**：当订单或变动事件的日期超出计划期（Horizon）的最大天数时，引擎自动在展期末端进行强行切断，忽略溢出段，确保内存不发生越界访问。