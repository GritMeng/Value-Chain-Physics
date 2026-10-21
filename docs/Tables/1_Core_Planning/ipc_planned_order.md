---
table_name: "ipc_planned_order"
alias: "planned_order"
module: "1_Core_Planning"
cpp_struct: "PlannedOrder"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 1_core_planning
---

# 🏷️ `ipc_planned_order` (planned_order)

> **业务说明**: 计划订单表。LBL-MRP 计算后生成的建议补货订单，包含计划开工/完工期、计划数量、物料/站点以及关联的维度组信息。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `planned_order` | planned_order | `VARCHAR(18)` | PK / NOT NULL | 计划订单号 |
| `request_start_date` | request_start_date | `DATE` | Nullable | 考虑了LT或者Constrain的最晚开始日期（采购和生产为单据开始日期，转储单为Built开始日期） |
| `due_date` | due_date | `DATE` | Nullable | 期望交付或就绪日期 |
| `eff_qty` | eff_qty | `DECIMAL(18,2)` | Nullable | 考虑过Yield,scrap,UNIT的之后的数量 |
| `dimension_grp` | dimension_grp | `VARCHAR(10)` | Nullable | 消纳维度组，用于 semiconductor binning 分级降级消纳规则匹配 |
| `is_planned` | is_planned | `VARCHAR(10)` | Nullable | 系统自动创建或是manual input |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `source` | source | `VARCHAR(10)` | Nullable | 关联MaterialSource |
| `qty` | qty | `DECIMAL(18,2)` | Nullable | 数量 (Quantity) |
| `ship_date` | ship_date | `DATE` | Nullable | 采购和转储订单 |
| `build_date` | build_date | `DATE` | Nullable | 采购和转储订单 |
| `base_unit` | base_unit | `VARCHAR(10)` | Nullable | 基准单位 |
| `planned_unit` | planned_unit | `VARCHAR(10)` | Nullable | 应用单位 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：有限能力补货指令生成与拆单
* **因果流向**：当 IOP 引擎的 CTP 递归探路成功（即资源与子件均齐套）后，求解器生成新的 `ipc_planned_order` 记录，以表示计划中的生产工单（Make）、采购计划（Buy）或调拨单（Transfer）。
* **工单生成与拆分编排**：
  1. 交期回推：若订单交期为 $D$，根据提前期 $LT$ 偏置推算开工期 $S = D - LT$。
  2. 产能扣减：在 $S$ 处的 `ipc_resource_capacity` 中锁定对应工时。若 $S$ 处产能不足，触发“产能拉平（Cap Leveling）”或“工单拆分（Order Split）”：
     - 系统自动将大工单拆分为多个子工单，分别落到相邻的可用产能天数上，生成多条 `PlannedOrder` 记录。
  3. 特征标记：在 `dimension_grp` 写入该订单的特征维度值，为下层子件分级消纳提供维度锁参数。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器内，计划订单以紧凑的 `PlannedOrder` 结构体进行一维连续存储。对于 DFS 回溯中临时产生的拆单行为，采用线程局部的 `PlannedOrderSplit` 向量栈进行零堆分配缓存：

```cpp
// 关联 ipc_planned_order 表的 C++ DOD 物理数据结构
struct PlannedOrder {
    uint32_t part_id;             // 物料 ID (对应 part)
    double qty;                   // 计划数量 (对应 qty)
    int start_day;                // 计划开工日期 (对应 request_start_date)
    int finish_day;               // 计划就绪交付日期 (对应 due_date)
    double dimension_val;         // 物料维度特征值 (对应 dimension_grp)
    int original_lbl_start = -1;  // 无约束条件下的理论开工期
    int original_lbl_finish = -1; // 无约束条件下的理论完工期
};

// DFS 有限能力排产事务中用于记录工单拆分 (Split) 的轻量栈结构
struct PlannedOrderSplit {
    size_t original_order_idx;    // 关联的原始计划订单索引
    double qty;                   // 拆分后的工单数量
    int finish_day;               // 拆分工单的完工期
    int start_day;                // 拆分工单的开工期
    double capacity;              // 占用的产能工时
    double routing_cost;          // 路线选择转产惩罚成本
};
```