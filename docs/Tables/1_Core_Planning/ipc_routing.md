---
table_name: "ipc_routing"
alias: "routing"
module: "1_Core_Planning"
cpp_struct: "OperationRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 1_core_planning
---

# 🏷️ `ipc_routing` (routing)

> **业务说明**: 工艺路线表。定义不同产品在各厂区站点下的加工路线，是有限产能派程和测试设备分配的输入基础。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `routing` | routing | `VARCHAR(10)` | PK / NOT NULL | Routing编号 |
| `description` | description | `VARCHAR` | Nullable | 描述 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：制造工步顺序编译
* **因果流向**：`ipc_routing` 定义了物料制造的工艺路线名称，在物理内存中它与工序明细级联，作为有限能力计算时资源负荷扣减的基础。
* **工步流程编排**：
  1. 工单生成时，根据 `routing` 展开其对应的全部操作工序（Operations）。
  2. 严格按 `sequence` 升序依次排定各工序的开工和完工期。
  3. 各工序占用的工作中心能力负荷，实时扣减 `ipc_resource_capacity`。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器内，工艺路线对应的具体工序明细被编译为连续物理内存空间内的 `OperationRecord` 数组，杜绝了树状链表遍历：

```cpp
// 关联工艺路线工序的 C++ DOD 物理数据结构
struct OperationRecord {
    std::string operation;    // 工序编码 (对应 operation)
    uint32_t sequence;        // 工序顺序号 (对应 sequence，如 10, 20, 30)
    std::string work_center;  // 加工该工序的工作中心 ID (对应 work_center)
    double setup_time = 0.0;  // 基础换型准备时间
    double run_time = 0.0;    // 单件加工工时 (对应 run_time)
    std::string routing;      // 工艺路线唯一编码 (对应 routing)
};
```