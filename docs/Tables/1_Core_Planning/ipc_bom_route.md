---
table_name: "ipc_bom_route"
alias: "part_routing_bom"
module: "1_Core_Planning"
cpp_struct: "ConstraintConsumption"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 1_core_planning
---

# 🏷️ `ipc_bom_route` (part_routing_bom)

> **业务说明**: 物料工艺BOM关系表（BOM Route）。将物料、工艺路线和BOM ID进行绑定，支持基于生效日期和优先级进行路由选择，特别包含维度组（dim_grp）以过滤匹配特定的多维分配路径。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `bomid` | bomid | `VARCHAR(40)` | PK / NOT NULL | BOM编号
Reference Table: [[ipc_bom_route|BOM]] |
| `routing` | routing | `VARCHAR(10)` | Nullable | Routing编号
Reference Table: [[ipc_routing|Routing]] |
| `dim_grp` | dim_grp | `VARCHAR(10)` | Nullable | Reference Table: [[ipc_demision_grp|Dimension]] |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | 有效开始日期
默认值：2020.8.31 |
| `eff_end_date` | eff_end_date | `DATE` | Nullable | 生效结束日期 2030.8.31 |
| `yield` | yield | `DECIMAL(18,2)` | Nullable | - |
| `max_qty` | max_qty | `DECIMAL(18,2)` | Nullable | - |
| `priority` | priority | `INTEGER` | Nullable | 优先级，数值越小越优先 |
| `bom_type` | bom_type | `VARCHAR(10)` | Nullable | Reference Table: [[ipc_part|Material]]BOMRoutingType |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多路径 CTP 选择与工艺分配
* **因果流向**：当 `ipc_planned_order` 生成制造补货需求时，求解器需要决定该工单运行在哪个设备/工艺路线（Routing）上。系统查询 `ipc_bom_route` 表，锁定该站点下该物料对应的可用工艺版本 `bomid` 与 `routing`。
* **工艺选择编排**：
  1. 优先级遍历：按 `priority` 从小到大依次尝试各路线。
  2. 有效期过滤：校验排产日期是否落在 `eff_start_date` 与 `eff_end_date` 区间内。
  3. 产能可用性校验：调用 CTP 引擎检测对应的 `ipc_resource_capacity` 天级负荷。若主路线产能超载，则递归选择优先级较低的替代工艺路线（Alternative Routing）。
  4. 特征维度匹配：读取 `dim_grp`，过滤仅匹配当前订单特征维度（如半导体分级）的特定分配路线。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 引擎中，每个物料站点的工艺和替代路线被编译为 SoA 对齐的备选路由数组，用于 DFS 递归时进行高速检索：

```cpp
// 关联 ipc_bom_route 的 C++ DOD 物理数据结构
struct ConstraintConsumption {
    uint32_t constraint_id;  // 资源约束ID (对应 ipc_constraint 表的主键)
    double factor;           // 产能消耗系数 (单位工时比率，对应 constraint_factor)
};

struct AlternativeRouting {
    uint32_t routing_id;                            // 替代路线逻辑ID
    std::vector<ConstraintConsumption> constraints; // 绑定的资源及产能消耗系数
    double routing_cost = 0.0;                      // 路线选择的惩罚/转产成本
    int priority = 0;                               // 优先级 (对应 priority)
};

// 对应 ipc_bom_route 的主约束记录体
struct SourceConstraintRecord {
    uint32_t part_id;
    uint32_t constraint_id;
    double constraint_factor;
    double before_fixed_factor; // 洗枪换型固定时间 (Setup Overhead)
    double after_fixed_factor;  // 清理固定工时 (Clean-up Overhead)
    
    std::vector<ConstraintConsumption> extra_constraints; // 伴生多约束
    std::vector<AlternativeRouting> alternative_routings;  // 备选替代工艺路线数组
};
```