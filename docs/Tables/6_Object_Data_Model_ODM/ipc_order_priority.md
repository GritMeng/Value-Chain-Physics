---
table_name: "ipc_order_priority"
alias: "order_priority"
module: "6_Object_Data_Model_ODM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_order_priority` (order_priority)

> **业务说明**: 包含的值可用于确定满足需求的顺序的优先级，或用于确定供应消耗的顺序的优先级。与DemandPriority表中每条记录相关联的优先级是通过其PlanningPriority字段定义的。DemandPriority和TransactionSequence(在IndependentDemand和schedulereceipt表中找到)一起操作以提供无限数量的优先级。也就是说，具有相同计划优先级和到期日期的需求然后按ExecutionSequence排序.
ExecutionSequence当IsCommited维护时可用

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `assignment_rule` | assignment_rule | `VARCHAR` | Nullable | 定义如何分配供应给到需求：
FIFO
FairShare
EqualShare |
| `descriotion` | descriotion | `VARCHAR` | Nullable | - |
| `is_commited` | is_commited | `VARCHAR` | Nullable | 指定事务序列是否被视为优先级的一部分，以便在新订单加入时维护现有订单的到期日。
Y - 已承诺的需求将具有较高优先级， ExectionSequence将参与到优先级排序中
N - ExectionSequence不参与 |
| `planning_priority` | planning_priority | `VARCHAR` | Nullable | 与每个DemandPriority值相关联的相对重要性。数值越低，有效优先级越高。
不同的DemandPriority记录可以共享相同的记录PlanningPriority价值。如果多个订单共享相同
PlanningPriority值，它们被视为相同的，即使它们的DemandPriority不同。因此，建议您给每个DemandPriority记录一个不同的名称PlanningPriority价值。 |
| `value` | value | `VARCHAR` | PK / NOT NULL | 唯一标识 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：意向预测与合同订单优先级位域划分
* **因果流向**：`ipc_order_priority` 定义了需求消纳时的“排他性原则”。当 `is_commited = true`（已承诺销售合同）时，系统激活严格的 Transactional Sequence 排队号，防止新单插单抢占老订单已承诺的交期。这与底层 64 位复合二进制优先级 `composite_priority` 编排强相关，通过位移控制把已承诺订单放在内存数组头部。