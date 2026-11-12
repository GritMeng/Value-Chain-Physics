---
table_name: "ipc_onhand"
alias: "oh_inventory"
module: "1_Core_Planning"
cpp_struct: "Axis"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 1_core_planning
---

# 🏷️ `ipc_onhand` (oh_inventory)

> **业务说明**: 物理在库库存表。记录每个库位、物料在不同站点和可用日期下的实际物理在库数量，是 LBL-MRP 供需消纳计算的库存初始水位。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `location` | location | `VARCHAR(10)` | PK / NOT NULL | 具体的物理库位编码 |
| `part` | part | `VARCHAR(40)` | 🔑 **PK / Required** | 物料唯一编码 (Part Code) |
| `site` | site | `VARCHAR(8)` | 🔑 **PK / Required** | 工厂或仓库站点编码 (Site Code，对应 SAP 的 Plant) |
| `available_date` | available_date | `DATE` | PK / NOT NULL | 实际供应可用或可承诺交付日期 (ATP Date) |
| `qty` | qty | `DECIMAL(18,2)` | Nullable | 数量 (Quantity) |
| `inventory_type` | inventory_type | `VARCHAR(10)` | Nullable | 库存类型 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：物理在库初始水位消纳
* **因果流向**：`ipc_onhand` 是 IOP 运行时，在 RunDate（T=0）时刻的库存物理快照。
* **冲抵算法编排**：
  1. 计算初始可用水位：在 MRP 消纳循环开始时，引擎抓取 `available_date` 小于等于计划期起始日的全部 `qty`，累加作为该 SKU 站点的初始在手水位 $CS_0$。
  2. 订单扣减：实际订单或预测通过 CTP 预占时，首先扣减 $CS_0$ 的在库现有量。
  3. 状态跃迁：扣减成功后，并不直接修改数据库，而是临时记录于事务栈中；待全部齐套并确认成交后，将扣减结果以 Planned Pegging 写入 `ipc_planned_supply_assignment` 并扣减 `ipc_onhand` 中的物理在库量。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在内存中，库存现有量并不是以离散的表行记录存储，而是深度融合在 `Axis`（水位数轴）中，并且通过**乐观无锁 SS 池化（Optimistic SS Pooling）**解决多渠道高并发扣减的锁竞争：

```cpp
// 对应 ipc_onhand 数据的内存数轴
struct Axis {
    std::vector<double> qtys;       // 离散事件发生数量 (D_i / S_j)
    std::vector<double> cum_qtys;   // 累加前缀和，表达水位高度 (C_i / A_j)
    std::vector<uint64_t> dates;    // 严格递增的事件日期轴 (以分钟/天表示)
};

// 乐观无锁库存分配共享池 (用以消除多渠道扣减时的物理表锁)
struct alignas(64) OptimisticInventoryPool {
    std::atomic<uint64_t> raw_bits; // 将 double 重新转译为 uint64_t 以实施原子操作
    
    // 采用 CAS (Compare-And-Swap) 乐观无锁扣减，达到单核数百万次/秒的并发分配吞吐
    bool allocate_stock_cas(double request_qty) {
        uint64_t current_bits = raw_bits.load(std::memory_order_relaxed);
        double current_val;
        uint64_t target_bits;
        double target_val;
        do {
            std::memcpy(&current_val, &current_bits, sizeof(double));
            if (current_val < request_qty) return false; // 库存不足，直接失败
            target_val = current_val - request_qty;
            std::memcpy(&target_bits, &target_val, sizeof(double));
        } while (!raw_bits.compare_exchange_weak(
            current_bits, target_bits,
            std::memory_order_release, std::memory_order_acquire
        ));
        return true;
    }
};
```