---
table_name: "ipc_service_level_target"
alias: "IO库存优化服务水平目标策略表"
module: "5_IO_Safety_Stock"
cpp_struct: "ServiceLevelTargetRecord"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 5_io_safety_stock
---

# 🏷️ `ipc_service_level_target` (IO库存优化服务水平目标策略表)

> **业务说明**: IO 安全库存目标策略表。设置不同客户段（如 VVIP / Normal）的安全库存服务水平目标，是计算动态库存防爆仓水位的关键输入。

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `part_code` | 物料编码 | `VARCHAR(100)` | 🔑 **PK / Required** | 联合主键，物料编码 (Part Code) |
| `site_code` | 站点编码 | `VARCHAR(100)` | 🔑 **PK / Required** | 联合主键，工厂或仓库站点编码 (Site Code) |
| `customer_segment` | 客户细分群体 | `VARCHAR(100)` | Nullable | 客户分段，如 VVIP / Normal |
| `service_level_target` | 服务水平目标 | `DOUBLE` | Nullable | 设定的库存优化服务水平目标 (如 0.98 代表 98% 交付率) |
| `lead_time_variance` | 提前期方差 | `DOUBLE` | Nullable | 物流提前期的波动偏差值，用于计算防爆仓安全库存水位 |


##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：多级安全库存优化（MEIO）方差传导与服务水平因子计算
* **因果流向**：`ipc_service_level_target` 设定了各物料-站点针对不同客户细分（Customer Segment）所要达到的准时交付服务目标，该目标直接决定了安全库存水位中服务因子 $Z$ 的大小。
* **计算算法编排**：
  1. 服务因子映射：引擎根据服务水平目标（如 95%）调用标准正态分布的逆累积分布函数（Inverse CDF），计算服务系数：
     $$ Z = \Phi^{-1}(service\_level\_target) $$
  2. 需求与提前期方差双重传导：结合物料本身的日需求均值 $\mu_D$，需求方差 $\sigma_D^2$，提前期均值 $L$ 以及提前期方差 $\sigma_L^2$（即本表的 `lead_time_variance`），计算总补货周期需求方差：
     $$ \sigma_{replenishment}^2 = L \cdot \sigma_D^2 + \mu_D^2 \cdot \sigma_L^2 $$
  3. 安全库存计算：
     $$ SS = Z \times \sigma_{replenishment} $$
  4. 多级安全库存优化（MEIO）：在此基础上，利用拉格朗日乘子法在多级仓储网络中寻求全局持有成本最低、同时满足最终端客户交付率的各节点服务水平组合。

###### 2. 物理内存结构设计 (C++ DOD Layout)
安全库存参数是 MRP 缺口计算的下限拦截水位。在 C++ 引擎中，为了支持 OpenMP 线程并行处理海量 SKU，`ServiceLevelTargetRecord` 被存储为连续的扁平结构，无缝配合时序数轴运算：
```cpp
// 对应 ipc_service_level_target 的内存对齐物理结构体
struct ServiceLevelTargetRecord {
    uint32_t part_id;                  // 物料ID (对应 part_code)
    uint32_t site_id;                  // 站点ID (对应 site_code)
    uint16_t segment_id;               // 客户细分类别ID
    double service_level_target;       // 交付目标比率 (对应 service_level_target)
    double z_factor;                   // 逆正态分布服务系数 (Z-score)
    double lead_time_variance;         // 提前期方差 (对应 lead_time_variance)
};
```

###### 3. 边界与异常处理
* **服务系数无穷大防护**：当用户录入的服务水平目标 $\ge 99.9\%$ 时，正态分布逆函数 $Z$ 将趋向无穷大（导致计算的安全库存为天文数字，资金链断裂）。引擎将 $Z$ 因子强制截断上限为 $4.0$（对应 99.99% 服务水平），以保护库存容量免于爆仓。
* **零需求静默物料**：当 SKU 历史需求方差 $\sigma_D^2$ 和均值均为 0 时，方差公式会得出 0 安全库存。若物料配置了强制服务水平，引擎会自动调用备选的 DOS 覆盖天数来设定最小库存底座。