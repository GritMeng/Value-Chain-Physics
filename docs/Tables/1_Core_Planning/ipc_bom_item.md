---
table_name: "ipc_bom_item"
alias: "bom_item"
module: "1_Core_Planning"
cpp_struct: "FlatBomItem"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 1_core_planning
---

# 🏷️ `ipc_bom_item` (bom_item)

> **业务说明**: BOM行项目明细表。定义组装件与子组件的父子拓扑关系，包含替代组（alt_grp）、分配优先级（priority）、替代迄今累计消耗量、目标占比（target）等核心替代控制字段。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `bomid` | bomid | `VARCHAR(10)` | Nullable | BOM编号 |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `component` | component | `VARCHAR(40)` | Nullable | 组件
Reference Table: [[ipc_part|Material]] |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | 生效日期 |
| `eff_end_date` | eff_end_date | `DATE` | Nullable | 失效日期 |
| `ratio` | ratio | `DECIMAL(18,2)` | Nullable | 采购比例，0-1表示还是0-100表示取决于MaterialBOMRouting.RatioRule |
| `perqty` | perqty | `DECIMAL(18,2)` | Nullable | 每单位assemble所用component的数量 |
| `scrap` | scrap | `DECIMAL(18,2)` | Nullable | 指定在组装的生产过程中丢失的组件的比例。例如，由于破损。 |
| `alt_grp` | alt_grp | `VARCHAR(10)` | Nullable | 替代组编码，相同替代组内的组件物料属于可替换物料 |
| `priority` | priority | `INTEGER` | Nullable | 优先级，数值越小越优先 |
| `alt_todate_qty` | alt_todate_qty | `DECIMAL(18,2)` | Nullable | 指示迄今为止已分配给此替代BOM的数量（例如，它可能代表已知的输入供应分配）。此值应以组件库存单位表示，并用于初始替代BOM级别的决策。此字段适用于替代BOM记录，其中altgrp.type.source_rule设置为“on_going”，并在确定一组中哪些可替代组件应生成计划订单以满足装配的依赖需求的逻辑中使用。 |
| `target` | target | `DECIMAL(18,2)` | Nullable | 确定在创建计划订单时，来自组装的依赖需求分解到替代组中的组件的比例或百分比。这个值应以组装件的供应单位来表示。然后，应该分解到组件的需求百分比被计算为组件的目标值除以组中所有组件的目标值之和。请注意，根据AlternateGroupType表上ComponentSourceRule字段中的设置，来自组装物料的每个依赖需求要么按比例在可替换组件之间分割，要么完全分配给可替换组件，否则该组件将远离其当前目标。如果计划的订单只应该在组中的主要组件上创建，那么可以在该字段中为其分配一个正值，并且应该为所有其他组件分配一个值0(零)。如果计划的订单应该均匀地分布在组中的所有组件上，那么可以在这个字段中为每个组件分配相同的正值(例如，可以指定值1)。 |
| `item` | item | `INTEGER` | Nullable | BOM行项目编号 |
| `operation` | operation | `VARCHAR(10)` | Nullable | 工序ID |
| `alt_bom` | alt_bom | `VARCHAR(10)` | Nullable | 替换的BOM版本号 |
| `alt_group` | alt_group | `VARCHAR(10)` | Nullable | 唯一标识，自动加1 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多级替代料 Lot-size 重归一化分摊
* **因果流向**：当 `ipc_planned_order` 生成组件净需求时，若发现主要组件缺料，系统触发替代判定。查询 `alt_grp` 不为空的 BOM 记录，锁定备选物料集合。
* **分摊算法编排**：
  1. 计算组内理论需求：根据各替代件的 `target`（占组内比例）与 `priority`（优先级）分摊净需求。
  2. 包装规格向上舍入：依据 `ipc_part_site` 的 `lot_size` 与当前 BOM 的 `scrap`（损耗率）对分配量向上取整：
     $$ Actual\_Qty = \lceil \frac{Due\_Qty \times perqty \times (1 + scrap)}{lot\_size} \rceil \times lot\_size $$
  3. 溢出残差抵消：因舍入导致的多余供应量，按 `perqty` 反向换算为父件等价值，抵消上层剩余净需求。
  4. 动态重归一化：将已决策件移出活跃集，对其余备选件的分配比例按剩余 `target` 重新计算分摊系数（Re-normalization）：
     $$ Ratio_i^{new} = \frac{target_i}{\sum_{k \in \text{Remaining}} target_k} $$
  5. 结果落库：写入 `ipc_planned_supply_assignment` 的 Pegging 分配明细，并在 `ipc_planned_order` 中记录具体被采购/制造的替代件供应。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 引擎中，为了避免在 BOM 树上反复跳转指针（Pointer Chasing）造成 L1/L2 Cache 失效，所有的 `ipc_bom_item` 表数据在初始化时，被一次性编译进一个**一维扁平、连续分布的 `flat_boms` 数组**中，其单条记录 `FlatBomItem` 在内存中严格对齐。

```cpp
// 对应 ipc_bom_item 表的 C++ DOD 物理对齐结构体 (255 bytes / 64 bytes cache-line aligned)
struct FlatBomItem {
    uint32_t parent_id;          // 父物料ID (由字符串哈希化或全局唯一的逻辑索引)
    uint32_t child_id;           // 子组件ID
    double per_qty;              // 每单位 assemble 消耗的子组件数量 (对应 perqty)
    double scrap;                // 制造损耗率 (对应 scrap)
    int alt_group_id = -1;       // 替代组逻辑编号 (对应 alt_group)
    int alt_priority = 0;        // 替代优先级 (对应 priority，值越小越优先)
    double target_ratio = 1.0;   // 目标分配比例 (对应 target)
    double historical_qty = 0.0; // 迄今为止累计消耗量 (对应 alt_todate_qty)
    double lot_size = 0.0;       // 包装规格批值 (对应 lot_size)
    uint8_t relation_op = 0;     // 关系操作符 (PASS=0, EQ=1...) 用于半导体分级
    double target_dim_val = 0.0; // 目标特征维度值 (用于 Dimension 映射)
    int eff_start_day = -1;      // 生效起始天数 (以 RunDate 为 0 的相对偏移天数)
    int eff_end_day = -1;        // 生效失效天数
    double ltb_limit = -1.0;     // 生命周期终期买入总量上限 (LTB Limit)
    int mix_group_id = -1;       // 排他性混合组ID (Mix Group ID)
    std::string relationship_type = "alt"; // 替代关系类型
};

// CSR (Compressed Sparse Row) 全网拓扑压缩结构
struct FlatTopology {
    // 偏移索引：Part_i 的子 BOM 行项目，在 flat_bom_items 数组中的偏移量为:
    // [ parent_to_bom_offsets[Part_i], parent_to_bom_offsets[Part_i + 1] )
    std::vector<size_t> parent_to_bom_offsets;
    std::vector<FlatBomItem> flat_bom_items;
};
```

###### 3. 边界与异常处理
* **BOM 环路判定（Loop detection）**：引擎在加载 `ipc_bom_item` 编译拓扑时，采用松弛算法更新各物料的 `low_level_code`。若出现 A -> B -> A 循环引用，且检测深度迭代超过 100 层，系统触发刚性报错熔断，防止 DFS 栈溢出。
* **BOM 有效期过滤**：在 CTP 递归预占时，系统会检查子件的 `eff_start_day` 与 `eff_end_day`。若排产日期超出有效期，系统自动跳过该 BOM 项，并触发替代料或报错流程。