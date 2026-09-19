# 测试数据 (Test Data)

本目录存放 IPC Core 引擎的**可复现测试数据**。所有数据均为 CSV 明文，可直接用文本编辑器或 pandas 查看、修改、替换。

## 数据文件清单

| 文件 | 规模 | 用途 | 对应引擎 |
| :--- | :--- | :--- | :--- |
| `parts.csv` | 2,000,000 行 | 物料-站点主数据 (SKU) | ATP / ITP / IOP / 替代料 |
| `demands_master.csv` | 500,000 行 | ITP 主计划需求（含缓冲系数前的原始需求） | ITP |
| `demands_execution.csv` | 500,000 行 | IOP 执行计划需求（含超量插单，触发阻断） | IOP |
| `bom.csv` | 12 行 | 产品结构（含四类替代料示例） | ATP / 替代料 |
| `atp_supply.csv` | 12 行 | 在手/在途/计划产出供给节点 | ATP / CTP |
| `capacity.csv` | 60 行 | 工作中心产能 | ATP / CTP |
| `substitution_group.csv` | 6 行 | 替代料组定义（Class 1/2/3） | 替代料 |
| `sample/` | 小规模 | 手工可读的迷你数据集，用于单步调试 | 全部 |

## 数据字典

### parts.csv
| 列 | 类型 | 说明 |
| :--- | :--- | :--- |
| `part_id` | uint32 | 物料唯一 ID（同时作为数组下标） |
| `part_code` | string | 物料编码 |
| `site` | string | 站点编码 |
| `safety_stock` | double | 安全库存（替代料消纳保护带） |
| `initial_on_hand` | double | 期初在手量 |
| `lot_size` | double | 批量倍数 |
| `lead_time` | int | 提前期（天） |

### demands_master.csv / demands_execution.csv
| 列 | 类型 | 说明 |
| :--- | :--- | :--- |
| `demand_id` | uint32 | 需求唯一 ID |
| `part_id` | uint32 | 需求物料 |
| `due_day` | int | 期望交期（天） |
| `qty` | double | 需求量 |
| `priority` | uint64 | 优先级（**数值越小优先级越高**） |
| `customer_group` | string | 客户组 |
| `region` | string | 销售大区 |

### bom.csv
| 列 | 类型 | 说明 |
| :--- | :--- | :--- |
| `parent_id` | uint32 | 父件物料 ID |
| `child_id` | uint32 | 子件物料 ID |
| `usage_qty` | double | 单位用量 |
| `alt_class` | uint8 | 替代等级：0=主料 1=一类 2=二类 3=三类 |
| `alt_group` | uint32 | 替代料组 ID |
| `target_ratio` | double | 目标配额比例 |
| `historical_qty` | double | 历史已累计分配量 |
| `lot_size` | double | 批量倍数 |

### atp_supply.csv
| 列 | 类型 | 说明 |
| :--- | :--- | :--- |
| `supply_code` | string | 供给节点编码 |
| `supply_type` | string | `On-Hand` / `SR` / `Planned-Order` |
| `part_id` | uint32 | 所属物料 |
| `available_day` | int | 可用日 |
| `qty` | double | 可用数量 |
| `priority` | uint64 | 供给优先级 |

### capacity.csv
| 列 | 类型 | 说明 |
| :--- | :--- | :--- |
| `work_center` | string | 工作中心编码 |
| `day` | int | 日期 |
| `capacity_hours` | double | 当日可用工时 |

### substitution_group.csv
| 列 | 类型 | 说明 |
| :--- | :--- | :--- |
| `alt_group` | uint32 | 替代料组 ID |
| `alt_class` | uint8 | 替代等级 |
| `member_part_id` | uint32 | 组内成员物料 ID |
| `target_ratio` | double | 目标配额比例 |
| `historical_qty` | double | 历史累计分配 |

## 数据生成规则（可复现）

大规模数据由 `data/generate_data.py` 确定性生成（固定 seed、纯函数推导），**不含随机性**，
因此每次生成结果完全一致。重新生成：

```bash
python3 data/generate_data.py            # 生成全部数据
python3 data/generate_data.py --sample   # 仅生成 sample/ 迷你集
```

规则摘要：
- `parts.csv`：200 万行，`part_id = i`，站点 `PLANT_MAIN`，安全库存 10，期初 100，批量 1，提前期 1
- `demands_master.csv`：50 万行，`part_id = i % 1_000_000`，`due_day = i % 30 + 1`，`qty = 50`，`priority = i % 5 + 1`
- `demands_execution.csv`：50 万行，**部分需求按 1.3 倍放大，用于触发 IOP 刚性阻断路径**（与 master 刻意不一致）

> ⚠️ 为什么 master 与 execution 不同？原 `stress_benchmark_2m.cpp` 两者共用同一份数据，
> 导致配额永远用不完、`blocked_orders` 恒为 0，阻断路径未被覆盖。本数据集刻意制造差异以覆盖阻断分支。
