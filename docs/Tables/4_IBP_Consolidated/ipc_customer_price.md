---
table_name: "ipc_customer_price"
alias: "customer_price"
module: "4_IBP_Consolidated"
cpp_struct: "PriceInterval"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 4_ibp_consolidated
---

# 🏷️ `ipc_customer_price` (customer_price)

> **业务说明**: 售卖价格

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `material_num` | material_num | `VARCHAR(40)` | PK / NOT NULL | 物料号 |
| `Customer` | customer | `VARCHAR(10)` | PK / NOT NULL | 客户编号 |
| `eff_start_date` | eff_start_date | `DATE` | Nullable | 开始日期 |
| `unit_price` | unit_price | `DECIMAL(18,2)` | Nullable | 单价 |
| `unit` | unit | `VARCHAR(10)` | Nullable | 单位 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多维阶梯式客户定价检索
* **因果流向**：在预测消纳与销售收入计算中，系统需要确定特定物料销售给特定客户的价格。系统查询 `ipc_customer_price`，根据物料、客户及交期匹配出最优的单价 `unit_price`，为 IBP 模块提供最终 Consensus Revenue 的计算依据。
* **匹配算法编排**：
  1. 客户定制价优先：优先检索物料与具体客户匹配且生效的记录。
  2. 通用价兜底：若无客户专属定价，则检索 `customer` 为空（即通配符）的记录作为渠道指导价。
  3. 平均售价（ASP）兜底：若前两步均未匹配，系统读取 `ipc_part` 中的 `average_selling_price` 字段。
  4. 阶梯数量优惠：根据订单的 `qty` 匹配对应的阶梯区间，获取优惠折扣后的最终单价。

###### 2. 物理内存结构设计 (C++ DOD Layout)
在 C++ 引擎中，每个物料站点的客户价格表被扁平化并按物料 ID 进行排序，形成时序价格区间段向量（Interval Vector），支持 $O(\log N)$ 的二分查找定位：
```cpp
// 单个价格生效周期
struct PriceInterval {
    int start_day;       // 生效开始天数 (对应 eff_start_date)
    int end_day;         // 生效失效天数
    double unit_price;   // 生效单价 (对应 unit_price)
};

// 对应 ipc_customer_price 的 C++ 内存表结构
struct CustomerPriceRecord {
    uint32_t part_id;                  // 物料ID (对应 material_num)
    uint32_t customer_id;              // 客户ID (对应 customer)
    std::vector<PriceInterval> prices;  // 按 start_day 排序的价格区间段
};
```

###### 3. 边界与异常处理
* **区间重叠冲突**：如果同一物料和客户在相同的时间区间录入了多条价格记录，引擎在编译阶段会报错并阻断，默认采用单价较高的一条作为保守财务估计。
* **历史回溯价格缺失**：当订单实际到期日（due_date）超出所有价格区间的上限或下限时，引擎会自动沿用最近的价格区间单价，避免计算收入为零。