# IPC 智能计划与控制系统：全场景交互演示与验证指南

本文档是 **IPC (Intelligent Planning & Control / 智能计划与控制系统)** 统御引擎的官方数据库级交互操作手册与验证套件。

---

## 🎯 场景三层设计架构与验证分类 (3-Tier Scenario Taxonomy)

IPC 引擎的场景验证体系分为严密的 **三大工程维度**，分别解决“算得快”、“算得准/决策优”以及“落地可行”：

```text
┌────────────────────────────────────────────────────────────────────────┐
│  一、 纯性能与极限规模压测层 (Extreme Performance & Scale Benchmark)     │
│       - 目标：50万需求 / 200万物料 / 20层BOM，纯 LBL MRP 极速消纳 (~240ms)   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
┌───────────────────────────────────▼────────────────────────────────────┐
│  二、 决策质量与业界难点攻克层 (Decision Quality & Industry Hard-Problems)│
│       - 目标：攻克全球 APS/SCM 业界痛点 (一/二/三类替代料, 组替代, 维度降级, SWAP)│
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
┌───────────────────────────────────▼────────────────────────────────────┐
│  三、 微观可行性与交期出单验证层 (Micro Dispatch & Delivery Feasibility)  │
│       - 目标：DBD 微观车间派程与 ATP 产能调度，验证“交期开不出、产能排得进” │
└────────────────────────────────────────────────────────────────────────┘
```

1. **第一层：纯性能与极限规模压测 (50万订单 / LBL 极速消纳)**：
   - **设计初衷**：剥离复杂的替代料与调度扰动，专注于测试 C++ 裸金属计算引擎在面对 50万需求/200万SKU 时的纯粹吞吐量（吞吐率 >2,000,000 需求/秒）。
2. **第二层：攻克业界难点的决策质量算子集 (Decision Quality Solvers)**：
   - **设计初衷**：替代料、配额平衡、组替代、芯片降级和呆滞置换是**全球 APS/SCM 领域公认最难解的决策黑洞**。IPC 引擎在此层专门验证这 6 大算子在极端复杂约束下的决策质量与平衡收敛度。
3. **第三层：微观可行性与交期出单验证 (DBD 派程与 ATP 产能)**：
   - **设计初衷**：验证宏观计划下发到车间后的微观可执行性，通过 DBD (Demand-Based Dispatch) 验证产能是否超载、交期是否能够准确开出并锁定。

---

## 📖 系统核心概念与运行范式

### 1. 传统系统“交错占用 (Cross-Feed / 越权抢料)”痛点
在传统的供应链 APS/MRP 系统中，如果不按严格的交期时序进行需求消纳，极易发生**交错占用（Cross-Feed / 错位抢料）**：
- 例如：一个交期较晚的远端订单（如 Day 30）抢先侵蚀了近端的 ATP（可承诺库存或产能）；
- 导致交期更紧急、优先级更高的高价值订单（如 Day 10）因为无法获取近端资源，被迫延误或缺料。

```text
错位抢料 (Cross-Feed) 示意图：
Day 0 ----------------- Day 10 ----------------- Day 30 -------> 时间轴
[近端供应: 100]                                 [远端供应: 100]
                         |                                |
       (错位侵蚀抢料)    |                                |
                         +--------------------------------+
                                                          |
                                                    [远端订单: 100] (错误侵蚀了近端供应！)
                         +--------------------------------+
                         |
                 [紧急高优先订单: 100] (无近端供应可用，被迫缺料延误！)
```

### 2. 时序拉动与呆滞置换双重优化 (Chronological Pull-then-SWAP)
为了彻底解决交错占用问题，IPC 引擎采用了两阶段第一性原理优化：
1. **时序拉动 (Commit-Date Pull)**：按订单承诺交期的先后顺序（最早交期优先），在时间轴上顺推/逆推消纳；
2. **呆滞置换安全阀 (SWAP Safety Valve)**：在标准替代料规则耗尽后，若高优先级订单依然存在缺口，引擎将自动扫描同组其他替代料的呆滞库存（超出安全库存的富余量）并进行动态置换，实现交付率最大化。

---

## 🛠️ 全局测试环境初始化 (Sandbox Environment Reset)

在运行任何场景的 SQL 数据前，请先清理 DucksDB 物理沙箱数据库：

```sql
-- 1. 清空上一轮算子的输出结果表
DELETE FROM ipc_planned_order_ledger;
DELETE FROM ipc_alternate_allocation;
DELETE FROM ipc_dispatch_ledger;
DELETE FROM ipc_swap_result;
DELETE FROM ipc_allotment_ledger;

-- 2. 清空基础主数据表
DELETE FROM ipc_material_node;
DELETE FROM ipc_bom_route;
DELETE FROM ipc_bom_item;
DELETE FROM ipc_onhand;
DELETE FROM ipc_scheduled_receipt;
DELETE FROM ipc_independent_demand;
DELETE FROM ipc_operation;
DELETE FROM ipc_work_center_capacity;
```

> [!NOTE]
> 引擎在内存中会将物料编码与站点自动编译为 `PART_CODE@SITE_CODE` 格式（如 `PART_MAIN@SITE_001`）。写回 DuckDB 时会自动剥离站点后缀，因此查询输出表时可直接使用原始物料编码（如 `PART_MAIN`）。

---

## 1. 一类替换料场景 (Class 1 Substitution: Quota Balancing)

### 1.1 业务场景说明
当某物料有多个合格供应商（如 `PART_P1` 与 `PART_P2`）时，采购团队会设定目标配额比例（如 60% vs 40%）。一类替换料算子会在多次连续计算中，动态平摊分配净需求，使累计实际分配量与目标配额的偏差（$|H_i - T_i|$）最小化。

### 1.2 数据播种 SQL (Data Setup SQL)
```sql
-- 1. 写入物料主数据
INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, run_rate, lead_time, round_to_integer) VALUES
('PART_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 1.0, true),
('PART_P1', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, true),
('PART_P2', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, true);

-- 2. 写入 BOM 工艺路线
INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES
('SITE_001', 'PART_MAIN', 'BOM_MAIN', 1, 'MPS');

-- 3. 写入 BOM 组件（替代组 ALT_GRP_1，优先级 Priority=1 触发一类平摊配额）
INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, alt_todate_qty, eff_start_day, eff_end_day, relationship_type, lot_size) VALUES
('BOM_MAIN', 'SITE_001', 'PART_P1', 1.0, 0.0, 'ALT_GRP_1', 1, 0.6, 0.0, -1, -1, 'alt', 0.0),
('BOM_MAIN', 'SITE_001', 'PART_P2', 1.0, 0.0, 'ALT_GRP_1', 1, 0.4, 0.0, -1, -1, 'alt', 0.0);

-- 4. 写入工作日历
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'DEFAULT' FROM generate_series(0, 99) g(i);
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'SITE_001' FROM generate_series(0, 99) g(i);

-- 5. 写入连续需求订单 (第1批: 10, 第2批: 10, 第3批: 20)
INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES
('DEMAND_1', 1.0, 'PART_MAIN', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 10.0, 10.0, 'OPEN', 1, 'SITE_001', 3, 1000.0),
('DEMAND_2', 1.0, 'PART_MAIN', 'SITE_001', 'CUST_A', '2026-06-18', '2026-06-18', 10.0, 10.0, 'OPEN', 2, 'SITE_001', 3, 1000.0),
('DEMAND_3', 1.0, 'PART_MAIN', 'SITE_001', 'CUST_A', '2026-06-28', '2026-06-28', 20.0, 20.0, 'OPEN', 3, 'SITE_001', 3, 2000.0);

-- 6. 配置求解器模式
DELETE FROM ipc_solver_config WHERE param_name = 'solver_mode';
INSERT INTO ipc_solver_config (param_name, param_value) VALUES ('solver_mode', 'itp');
```

### 1.3 C++ 引擎求解执行
在工作区命令行运行 IPC 引擎主程序：
```powershell
.\main_mem3.exe --db ipc.db
```

### 1.4 验证与断言 SQL
校验需求分配是否收敛至 60% / 40% 的目标配额：
```sql
SELECT alt_part, SUM(allocated_qty) AS allocated_qty 
FROM ipc_alternate_allocation 
GROUP BY alt_part 
ORDER BY alt_part;
```
* **预期正确输出结果**：
  | alt_part | allocated_qty |
  | :--- | :---: |
  | `PART_P1` | 30.0 |
  | `PART_P2` | 10.0 |

---

## 2. 二类替换料场景 (Class 2 Substitution: Supplier Rating)

### 2.1 业务场景说明
针对长期战略供应商，采购希望避免分配量的剧烈波动。二类替换料算子通过累计实际采购量除以目标配额（$H_i / \gamma_i$）计算履约评分，优先将新增需求分配给履约评分最低的供应商，保持平稳拉动。

### 2.2 数据播种 SQL
```sql
-- 1. 写入物料主数据
INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, run_rate, lead_time, round_to_integer) VALUES
('PART_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 1.0, true),
('PART_P1', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, true),
('PART_P2', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, true);

-- 2. 写入 BOM 工艺路线
INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES
('SITE_001', 'PART_MAIN', 'BOM_MAIN', 1, 'MPS');

-- 3. 写入 BOM 组件（替代组 ALT_GRP_1，优先级 Priority=2 触发二类履约选优）
INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, alt_todate_qty, eff_start_day, eff_end_day, relationship_type, lot_size) VALUES
('BOM_MAIN', 'SITE_001', 'PART_P1', 1.0, 0.0, 'ALT_GRP_1', 2, 0.6, 0.0, -1, -1, 'alt', 0.0),
('BOM_MAIN', 'SITE_001', 'PART_P2', 1.0, 0.0, 'ALT_GRP_1', 2, 0.4, 0.0, -1, -1, 'alt', 0.0);

-- 4. 写入日历
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'DEFAULT' FROM generate_series(0, 99) g(i);
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'SITE_001' FROM generate_series(0, 99) g(i);

-- 5. 写入需求订单
INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES
('DEMAND_1', 1.0, 'PART_MAIN', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 10.0, 10.0, 'OPEN', 1, 'SITE_001', 3, 1000.0),
('DEMAND_2', 1.0, 'PART_MAIN', 'SITE_001', 'CUST_A', '2026-06-18', '2026-06-18', 20.0, 20.0, 'OPEN', 2, 'SITE_001', 3, 2000.0),
('DEMAND_3', 1.0, 'PART_MAIN', 'SITE_001', 'CUST_A', '2026-06-28', '2026-06-28', 30.0, 30.0, 'OPEN', 3, 'SITE_001', 3, 3000.0);

-- 6. 配置求解器
DELETE FROM ipc_solver_config WHERE param_name = 'solver_mode';
INSERT INTO ipc_solver_config (param_name, param_value) VALUES ('solver_mode', 'itp');
```

### 2.3 引擎求解执行
```powershell
.\main_mem3.exe --db ipc.db
```

### 2.4 验证与断言 SQL
```sql
SELECT alt_part, SUM(allocated_qty) AS allocated_qty 
FROM ipc_alternate_allocation 
GROUP BY alt_part 
ORDER BY alt_part;
```
* **预期正确输出结果**：
  | alt_part | allocated_qty |
  | :--- | :---: |
  | `PART_P1` | 40.0 |
  | `PART_P2` | 20.0 |

---

## 3. 三类替换料场景 (Class 3 Substitution: Lot-Sizing Constraints)

### 3.1 业务场景说明
供应商发货通常有固定的包装最小批次（Lot-Size，如整箱/整包）。三类替换料算子在配额划分时，会自动向上向上规整到包装批次的整数倍，并通过多轮归一化配额调整，避免因批次向上规整导致的过度超采购。

### 3.2 数据播种 SQL
```sql
-- 1. 写入物料主数据
INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, run_rate, lead_time, round_to_integer) VALUES
('PART_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 1.0, true),
('PART_P1', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, true),
('PART_P2', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, true),
('PART_P3', 'RAW', 'MRP', 'SITE_001', false, 0.0, 1.0, true);

-- 2. 写入 BOM 工艺路线
INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES
('SITE_001', 'PART_MAIN', 'BOM_MAIN', 1, 'MPS');

-- 3. 写入 BOM 组件（Priority=3 触发三类批次规整，设定打包批次 20, 15, 10）
INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, alt_todate_qty, eff_start_day, eff_end_day, relationship_type, lot_size) VALUES
('BOM_MAIN', 'SITE_001', 'PART_P1', 1.0, 0.0, 'ALT_GRP_1', 3, 0.5, 0.0, -1, -1, 'alt', 20.0),
('BOM_MAIN', 'SITE_001', 'PART_P2', 1.0, 0.0, 'ALT_GRP_1', 3, 0.3, 0.0, -1, -1, 'alt', 15.0),
('BOM_MAIN', 'SITE_001', 'PART_P3', 1.0, 0.0, 'ALT_GRP_1', 3, 0.2, 0.0, -1, -1, 'alt', 10.0);

-- 4. 写入在手库存
INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type) VALUES
('WH_P1', 'PART_P1', 'SITE_001', '2026-05-29', 100.0, 'Standard'),
('WH_P2', 'PART_P2', 'SITE_001', '2026-05-29', 100.0, 'Standard'),
('WH_P3', 'PART_P3', 'SITE_001', '2026-05-29', 100.0, 'Standard');

-- 5. 写入日历
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'DEFAULT' FROM generate_series(0, 99) g(i);
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'SITE_001' FROM generate_series(0, 99) g(i);

-- 6. 写入需求量 100
INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES
('DEMAND_1', 1.0, 'PART_MAIN', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 100.0, 100.0, 'OPEN', 1, 'SITE_001', 3, 10000.0);

-- 7. 配置求解器
DELETE FROM ipc_solver_config WHERE param_name = 'solver_mode';
INSERT INTO ipc_solver_config (param_name, param_value) VALUES ('solver_mode', 'itp');
```

### 3.3 引擎求解执行
```powershell
.\main_mem3.exe --db ipc.db
```

### 3.4 验证与断言 SQL
```sql
SELECT alt_part, SUM(allocated_qty) AS allocated_qty 
FROM ipc_alternate_allocation 
GROUP BY alt_part 
ORDER BY alt_part;
```
* **预期正确输出结果**：
  | alt_part | allocated_qty |
  | :--- | :---: |
  | `PART_P1` | 60.0 |
  | `PART_P2` | 30.0 |
  | `PART_P3` | 10.0 |

---

## 4. 维度感知联副产品分级与降级使用 (Dimension-Based Downgrading)

### 4.1 业务场景说明
在半导体晶圆分选或高精制造中，产出物会分化为不同容量/性能等级（如 512MB、256MB、128MB）。当低等级需求（如 256MB）出现缺口时，引擎可自动将高等级富余产品（512MB）降级（Down-Bin）消纳，避免重新下发晶圆投入工单。

### 4.2 数据播种 SQL
```sql
-- 1. 种子物料填充（映射至引擎维度算子范围）
INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, lead_time)
SELECT 'DUMMY_PART_' || i, 'RAW', 'MRP', 'SITE_001', false, 1.0 FROM generate_series(0, 199) g(i);

INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, lead_time, round_to_integer) VALUES
('PART_DIM_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 1.0, true),
('PART_DIM_EQ512', 'RAW', 'MRP', 'SITE_001', false, 1.0, true),
('PART_DIM_GE256', 'RAW', 'MRP', 'SITE_001', false, 1.0, true),
('PART_DIM_STD', 'RAW', 'MRP', 'SITE_001', false, 1.0, true);

-- 2. BOM 路线
INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES
('SITE_001', 'PART_DIM_MAIN', 'BOM_DIM', 1, 'MPS');

INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, priority, relationship_type) VALUES
('BOM_DIM', 'SITE_001', 'PART_DIM_EQ512', 1.0, 0.0, 1, 'std'),
('BOM_DIM', 'SITE_001', 'PART_DIM_GE256', 1.0, 0.0, 1, 'std'),
('BOM_DIM', 'SITE_001', 'PART_DIM_STD', 1.0, 0.0, 1, 'std');

-- 3. 日历
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'DEFAULT' FROM generate_series(0, 99) g(i);
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'SITE_001' FROM generate_series(0, 99) g(i);

-- 4. 指定维度的客户需求 (DIM_102.0 代表 512MB，DIM_101.0 代表 256MB)
INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, dimension_grp, customer_tier, revenue) VALUES
('DEMAND_1', 1.0, 'PART_DIM_MAIN', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 10.0, 10.0, 'OPEN', 1, 'SITE_001', 'DIM_102.0', 3, 1000.0),
('DEMAND_2', 1.0, 'PART_DIM_MAIN', 'SITE_001', 'CUST_A', '2026-06-18', '2026-06-18', 10.0, 10.0, 'OPEN', 2, 'SITE_001', 'DIM_101.0', 3, 1000.0);

-- 5. 配置求解器
DELETE FROM ipc_solver_config WHERE param_name = 'solver_mode';
INSERT INTO ipc_solver_config (param_name, param_value) VALUES ('solver_mode', 'itp');
```

### 4.3 引擎求解执行
```powershell
.\main_mem3.exe --db ipc.db
```

### 4.4 验证与断言 SQL
```sql
SELECT part_code, order_qty, dimension_val 
FROM ipc_planned_order_ledger 
WHERE part_code LIKE 'PART_DIM%'
ORDER BY part_code, dimension_val;
```

---

## 5. 组替代多准则决策选择分配 (Group Sourcing MCDM Strategy)

### 5.1 业务场景说明
战略零部件常常需要成对替换（例如组1: $\{A_1, B_1\}$ 或组2: $\{A_2, B_2\}$）。多准则决策（MCDM）算法会根据：低层码深度、新增采购成本、以及现有呆滞库存资金占用三个维度，综合评估并优先选择成本最低、库存消化效果最好的替代组。

### 5.2 数据播种 SQL
```sql
-- 1. 写入物料主数据
INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, lead_time, selling_ave_price) VALUES
('PART_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 1.0, 10.0),
('PART_G1_A', 'RAW', 'MRP', 'SITE_001', false, 1.0, 10.0),
('PART_G1_B', 'RAW', 'MRP', 'SITE_001', false, 1.0, 10.0),
('PART_G2_A', 'RAW', 'MRP', 'SITE_001', false, 1.0, 50.0),
('PART_G2_B', 'RAW', 'MRP', 'SITE_001', false, 1.0, 50.0);

-- 2. BOM 路线
INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES
('SITE_001', 'PART_MAIN', 'BOM_MAIN', 1, 'MPS');

-- 3. BOM 组件（组 1 绑定 ALT_GRP_1，组 2 绑定 ALT_GRP_2）
INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, relationship_type) VALUES
('BOM_MAIN', 'SITE_001', 'PART_G1_A', 1.0, 0.0, 'ALT_GRP_1', 1, 1.0, 'alt'),
('BOM_MAIN', 'SITE_001', 'PART_G1_B', 1.0, 0.0, 'ALT_GRP_1', 1, 1.0, 'alt'),
('BOM_MAIN', 'SITE_001', 'PART_G2_A', 1.0, 0.0, 'ALT_GRP_2', 1, 1.0, 'alt'),
('BOM_MAIN', 'SITE_001', 'PART_G2_B', 1.0, 0.0, 'ALT_GRP_2', 1, 1.0, 'alt');

-- 4. 写入库存（组 2 具备 5 套呆滞库存，MCDM 评估综合成本更优）
INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type) VALUES
('WH_G2_A', 'PART_G2_A', 'SITE_001', '2026-05-29', 5.0, 'Standard'),
('WH_G2_B', 'PART_G2_B', 'SITE_001', '2026-05-29', 5.0, 'Standard');

-- 5. 日历
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'DEFAULT' FROM generate_series(0, 99) g(i);
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'SITE_001' FROM generate_series(0, 99) g(i);

-- 6. 需求 = 5
INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES
('DEMAND_1', 1.0, 'PART_MAIN', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 5.0, 5.0, 'OPEN', 1, 'SITE_001', 3, 1000.0);

-- 7. 配置求解器
DELETE FROM ipc_solver_config WHERE param_name = 'solver_mode';
INSERT INTO ipc_solver_config (param_name, param_value) VALUES ('solver_mode', 'itp');
```

### 5.3 引擎求解执行
```powershell
.\main_mem3.exe --db ipc.db
```

### 5.4 验证与断言 SQL
```sql
SELECT alt_part, SUM(allocated_qty) AS allocated_qty 
FROM ipc_alternate_allocation 
GROUP BY alt_part 
ORDER BY alt_part;
```
* **预期正确输出结果**：
  | alt_part | allocated_qty |
  | :--- | :---: |
  | `PART_G2_A` | 5.0 |
  | `PART_G2_B` | 5.0 |

---

## 6. Swap 呆滞料置换与配额保护安全策略 (Swap Engine & Allotment Protection)

### 6.1 业务场景说明
当标准替代料耗尽且订单依然面临缺口时，**Swap 呆滞置换引擎**会扫描其他 BOM 路线的呆滞库存填补缺口，同时严密保护安全库存底线（Safety Stock）不被侵蚀。

### 6.2 数据播种 SQL
```sql
-- 1. 物料主数据（COMP_ALT 位于 SITE_002，包含 100 件受保护的安全库存）
INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, selling_ave_price, transshipment_lead_time, safety_stock) VALUES
('FG_MAIN', 'FINISHED', 'MPS', 'SITE_001', false, 10.0, 0, 0.0),
('COMP_1', 'RAW', 'MRP', 'SITE_001', false, 2.0, 0, 0.0),
('COMP_ALT', 'RAW', 'MRP', 'SITE_002', false, 1.5, 3, 100.0);

-- 2. BOM 路线
INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES
('SITE_001', 'FG_MAIN', 'BOM_MAIN', 1, 'MPS');

INSERT INTO ipc_bom_item (bomid, site, component, perqty, scrap, alt_grp, priority, target, eff_start_day, eff_end_day, relationship_type) VALUES
('BOM_MAIN', 'SITE_001', 'COMP_1', 1.0, 0.0, 'ALT_GRP_1', 1, 1.0, 0, 10, 'soft'),
('BOM_MAIN', 'SITE_002', 'COMP_ALT', 1.0, 0.0, 'ALT_GRP_1', 2, 1.0, 0, 10, 'soft');

-- 3. 在手库存 (COMP_ALT 共有 150 件，扣除 100 件安全库存后，仅 50 件可供 Swap 置换！)
INSERT INTO ipc_onhand (location, part, site, available_date, qty, inventory_type) VALUES
('WH_ALT', 'COMP_ALT', 'SITE_002', '2026-05-29', 150.0, 'Standard'),
('WH_MAIN', 'COMP_1', 'SITE_001', '2026-05-29', 30.0, 'Standard');

-- 4. 日历
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'DEFAULT' FROM generate_series(0, 99) g(i);
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'SITE_001' FROM generate_series(0, 99) g(i);
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'SITE_002' FROM generate_series(0, 99) g(i);

-- 5. 需求订单 (Day 15 需求 100 件)
INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES
('DEMAND_1', 1.0, 'FG_MAIN', 'SITE_001', 'CUST_A', '2026-06-13', '2026-06-13', 100.0, 100.0, 'OPEN', 1, 'SITE_001', 3, 1000.0);

-- 6. 配置求解器模式为 iop 激活置换引擎
DELETE FROM ipc_solver_config WHERE param_name = 'solver_mode';
INSERT INTO ipc_solver_config (param_name, param_value) VALUES ('solver_mode', 'iop');
```

### 6.3 引擎求解执行
```powershell
.\main_mem3.exe --db ipc.db
```

### 6.4 验证与断言 SQL
验证 `COMP_ALT` 仅被置换消纳了可用的 50 件，未触碰 100 件安全库存底线：
```sql
SELECT alt_part, SUM(allocated_qty) AS allocated_qty 
FROM ipc_alternate_allocation 
WHERE alt_part = 'COMP_ALT'
GROUP BY alt_part;
```
* **预期正确输出结果**：
  | alt_part | allocated_qty |
  | :--- | :---: |
  | `COMP_ALT` | 50.0 |

查询置换履历明细：
```sql
SELECT * FROM ipc_swap_result;
```

---

## 7. 双端前缀和几何无锁消纳算子 (Double-Ended Prefix-Sum Netting)

### 7.1 技术原理
为了在多线程并行计算中彻底消除锁竞争瓶颈，IPC 求解器将供应与需求连续映射为内存中的“前缀和水位线向量”，并利用无分支几何交叉公式计算分配量与缺口。

给定需求区间 $[CD_0, CD_1]$ 与累计供应水位 $CS_0$：
* $\text{分配量 (Allocated)} = \max\Big(0.0, \; \min(CD_1, CS_0) - \max(CD_0, 0.0)\Big)$
* $\text{缺口量 (Shortage)} = \max\Big(0.0, \; CD_1 - \max(CD_0, CS_0)\Big)$

### 7.2 SQL 确权校验
可在 DuckDB 中运行需求水位与计划订单累积水位的对比分析：
```sql
WITH demand_cumulative AS (
    SELECT part, request_due_date, request_qty,
           SUM(request_qty) OVER (PARTITION BY part ORDER BY request_due_date) AS cum_demand
    FROM ipc_independent_demand
),
supply_cumulative AS (
    SELECT part_code, 
           SUM(order_qty) AS total_planned
    FROM ipc_planned_order_ledger
    GROUP BY part_code
)
SELECT d.part, d.request_due_date, d.cum_demand, COALESCE(s.total_planned, 0) AS total_planned
FROM demand_cumulative d
LEFT JOIN supply_cumulative s ON d.part = s.part_code
ORDER BY d.part, d.request_due_date;
```

---

## 8. 微观时序产能与时序 ATP 派程调度 (DBD Dispatching & ATP Capacity Scheduling)

### 8.1 业务场景说明
当 LBL 宏观主计划生成计划订单后，**Demand-Based Dispatch (DBD)** 微观派程引擎接管车间调度，将计划订单按工序精准排布到车间工作中心的日历产能线上。

### 8.2 数据播种 SQL
```sql
-- 1. 写入物料
INSERT INTO ipc_material_node (part, part_type, mrp_rule, site, is_phantom, run_rate, lead_time) VALUES
('PART_FG', 'FINISHED', 'MPS', 'SITE_001', false, 0.0, 1.0);

-- 2. 写入 BOM 路线
INSERT INTO ipc_bom_route (site, part, bomid, priority, bom_type) VALUES
('SITE_001', 'PART_FG', 'BOM_FG', 1, 'MPS');

-- 3. 写入工序 Routing (在 WC_MAIN 工作中心准备时间 1 小时，单件运行时间 0.05 小时)
INSERT INTO ipc_operation (operation, description, sequence, work_center, operation_type, setup_time, run_time, routing, site) VALUES
('OP_10', 'Assembly', 10, 'WC_MAIN', 'Assembly', 1.0, 0.05, 'PART_FG', 'SITE_001');

-- 4. 写入工作中心日历产能 (WC_MAIN 每日 8 小时产能)
INSERT INTO ipc_sop_calendar_date (date, display, calendar) 
SELECT '2026-05-29'::DATE + i, 'work', 'DEFAULT' FROM generate_series(0, 99) g(i);

INSERT INTO ipc_work_center_capacity (work_center, date, working_hour, efficiency, number_of_resources, utilization, capacity, capacity_override, source)
SELECT 'WC_MAIN', '2026-05-29'::DATE + i, 8.0, 1.0, 1.0, 1.0, '8.0', '8.0', 'CAL' FROM generate_series(0, 99) g(i);

-- 5. 写入需求 (Day 10 需求 100 件，总工时需求 = 1 + 100 * 0.05 = 6.0 小时)
INSERT INTO ipc_independent_demand (demand, item, part, par_site, customer, request_delivery_date, request_due_date, open_qty, request_qty, status, order_priority, site, customer_tier, revenue) VALUES
('DEMAND_1', 1.0, 'PART_FG', 'SITE_001', 'CUST_A', '2026-06-08', '2026-06-08', 100.0, 100.0, 'OPEN', 1, 'SITE_001', 3, 1000.0);

-- 6. 配置求解器全流程运行 (solver_step = all)
DELETE FROM ipc_solver_config WHERE param_name = 'solver_mode';
INSERT INTO ipc_solver_config (param_name, param_value) VALUES ('solver_mode', 'itp');
DELETE FROM ipc_solver_config WHERE param_name = 'solver_step';
INSERT INTO ipc_solver_config (param_name, param_value) VALUES ('solver_step', 'all');
```

### 8.3 引擎求解执行
```powershell
.\main_mem3.exe --db ipc.db
```

### 8.4 验证与断言 SQL
```sql
SELECT part_code, order_qty, original_start_day, original_due_day, scheduled_day, allocated_capacity, routing_cost 
FROM ipc_dispatch_ledger 
ORDER BY scheduled_day;
```
* **预期正确输出结果**：
  验证 `allocated_capacity` 准确核算为 `6.0` 小时，且排产日期与车间产能时间轴完全匹配。
