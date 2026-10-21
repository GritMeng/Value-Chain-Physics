---
table_name: "ipc_financial_ledger"
alias: "IBP集成财务分类账表"
module: "4_IBP_Consolidated"
cpp_struct: "FinancialLedgerRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_financial_ledger` (IBP集成财务分类账表)

> **业务说明**: IBP 集成财务总账表。汇集总收入、库存持有成本、采购总成本等宏观财务指标，支持沙盘对比与财务联动对账。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `scenario_code` | 沙盘场景编码 | `VARCHAR(100)` | 🔑 **PK / Required** | 联合主键，沙盘场景唯一编码，如 baseline 或沙盘 ID |
| `period_code` | 会计期间编码 | `VARCHAR(50)` | 🔑 **PK / Required** | 联合主键，如 M1, M2 滚动视界 |
| `total_revenue` | 共识总营业收入 | `DOUBLE` | Nullable | 场景总共识营业收入额 |
| `inventory_carrying_cost` | 库存持有成本 | `DOUBLE` | Nullable | 计划重算后的库存资金占用与持有成本汇总值 |
| `purchasing_cost` | 采购总成本 | `DOUBLE` | Nullable | 物料采购总支出额 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：级联受损自底向上财务实时对账
* **因果流向**：`ipc_financial_ledger` 汇总了全网重算后的宏观财务健康指标。当供应链底层（如供应商或加工中心）发生延迟中断时，系统进行微秒级财务穿透。
* **级联受损重算与财务同步编排**：
  1. 影响传播：当底层零部件 $C$ 损失数量 $Qty_{lost}$，引擎通过 CSC 有向拓扑索引（`FlatTopology.flat_child_bom_indices`）以 LLC 升序自底向上逐层松弛，计算传导至最顶层成品 $P$ 的受损量：
     $$ Qty_{lost}^{parent} = \frac{Qty_{lost}^{child}}{per\_qty \times (1 + scrap)} $$
  2. 订单扣减与惩罚：扣减对应 `ipc_sales_order_line` 的有效交货量，计算延期交付罚金与营收流失（Lost Revenue）。
  3. 物理账本更新：引擎自动组合增量财务数据，利用 DuckDB 写事务（Write Transaction）实时执行列式更新，重算该沙箱场景下的 `total_revenue` 与 `inventory_carrying_cost`（包含新生成的计划订单库存资金占用）。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 求解器向 DuckDB 物理表同步时，为了避免高频的 SQL 拼接与解析，IPC 采用**DuckDB 物理二进制追加器（`duckdb::Appender`）**。求解器线程在内存中完成级联计算后，以连续数组的形式，通过零拷贝内存映射直接写盘：

```cpp
// 财务账本同步暂存结构 (C++ DOD Layout)
struct FinancialLedgerRecord {
    char scenario_code[100];
    char period_code[50];
    double total_revenue;
    double inventory_carrying_cost;
    double purchasing_cost;
};
```