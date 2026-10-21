---
table_name: "ipc_part_sequence"
alias: "part_sequence"
module: "6_Object_Data_Model_ODM"
cpp_struct: "PartSequenceRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 6_object_data_model_odm
---

# 🏷️ `ipc_part_sequence` (part_sequence)

> **业务说明**: 标识在wheel中分配给物料的顺序

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `part` | part | `VARCHAR` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `wheel` | wheel | `VARCHAR` | PK / NOT NULL | Reference:Wheel |
| `sequence` | sequence | `VARCHAR` | Nullable | 越小优先级越高 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：瓶颈轮转排产顺序控制
* **因果流向**：`ipc_part_sequence` 规定了在 `ipc_wheel`（生产轮）上各种物料的排产顺序优先级。
* **算法编排**：当多笔工单并发申请同一条生产线的产能时，除了交期外，求解器参考 `sequence` 指标进行重排，促使相同或相邻顺序的物料工单集中生产，最大限度压缩瓶颈测试设备的洗枪换型工时。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 中，轮式排产顺序以扁平数组存放，在 DFS 排程时直接作为物料排序的静态因子参与计算，耗时为 $O(1)$：
```cpp
struct PartSequenceRecord {
    uint32_t part_id;
    uint32_t wheel_id;
    uint32_t sequence_num; // 顺序编号 (值越小越优先)
};
```