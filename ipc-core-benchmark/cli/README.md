# IPC Engine CLI 契约

`ipc_engine_cli` 为既有 IPC Core 引擎提供一个稳定的「输入 CSV 目录 + 输出 CSV 目录」
进程边界，供 Node.js 编排层调用。本 CL**为新增**，不修改既有引擎算子、头文件契约、
既有基准程序与既有数据文件。

## 构建

```bash
./build_and_run.sh          # 一键编译并运行基准，产出 bin/ipc_engine_cli
# 或使用 CMake
cmake -S . -B build && cmake --build build --target ipc_engine_cli
```

## 用法

```bash
ipc_engine_cli --engine <delivery|itp|iop|substitution> \
               --in <输入目录> --out <输出目录> [选项]
```

`--in` 可直接指向一个**测试场景目录**（含七类数据集 CSV），无需任何格式转换。
输出目录必须已存在；每个引擎写出各自的输出 CSV（见下）。

### 通用选项

| 选项 | 适用引擎 | 默认 | 说明 |
| :--- | :--- | :--- | :--- |
| `--engine` | 全部 | 必填 | `delivery` / `itp` / `iop` / `substitution` |
| `--in` | 全部 | 必填 | 输入 CSV 目录 |
| `--out` | 全部 | 必填 | 输出 CSV 目录 |
| `--part-id` | delivery | 0 | 查询物料 ID |
| `--due-day` | delivery | 3 | 期望交期（天） |
| `--qty` | delivery | 30 | 需求数量 |
| `--priority` | delivery | 1 | 优先级 |
| `--buffer-factor` | itp | 1.10 | 主计划缓冲系数 |
| `--net-demand` | substitution | 50 | 净需求 |
| `--alt-class` | substitution | 1 | 替代料类别 `1｜2｜3` |
| `--alt-group` | substitution | 1 | 替代料组 ID |
| `--day` | substitution | 1 | 分配日 |
| `--parent-id` | substitution | 0 | 父物料 ID |
| `-h, --help` | 全部 | — | 打印用法 |

## 输入 CSV 契约（场景目录）

场景目录须包含七类数据集，列契约与 `data/README.md` 一致：

| 文件 | 用于引擎 | 列 |
| :--- | :--- | :--- |
| `parts.csv` | 全部 | `part_id,part_code,site,safety_stock,initial_on_hand,lot_size,lead_time` |
| `bom.csv` | delivery, substitution | `parent_id,child_id,usage_qty,alt_class,alt_group,target_ratio,historical_qty,lot_size` |
| `atp_supply.csv` | delivery | `supply_code,supply_type,part_id,available_day,qty,priority` |
| `capacity.csv` | delivery | `work_center,day,capacity_hours` |
| `demands_master.csv` | itp | `demand_id,part_id,due_day,qty,priority,customer_group,region` |
| `demands_execution.csv` | iop | 同上 |
| `substitution_group.csv` | substitution | `alt_group,alt_class,member_part_id,target_ratio,historical_qty` |

`delivery` 仅需 `parts/bom/atp_supply/capacity`；`itp` 需 `parts/demands_master`；
`iop` 需 `parts/demands_execution`（内部先生成 ITP 配额再跑 IOP）；`substitution`
需 `parts/bom`。

## 输出 CSV 契约

### delivery

- `delivery_result.csv` — 单行结论
  `part_id,due_day,qty,priority,is_fulfillable,promised_day,promised_qty,total_capacity_used,rollback_steps_count`
- `delivery_steps.csv` — 逐候选日试算与回滚
  `attempt_day,success,capacity_used,rollback_steps,note`
- `delivery_bom.csv` — 逐层 BOM 展开
  `level_parent_id,level_child_id,usage_qty,child_required_qty,alt_class,alt_group`

### itp

- `itp_allotments.csv`
  `day,family_id,cust_group_id,region_id,total_quota,consumed_qty`

### iop

- `iop_orders.csv` — 逐单明细（含约束键与阻断原因）
  `demand_id,part_id,due_day,qty,priority,family_id,cust_group_id,region_id,status,reason`
- `iop_summary.csv` — 聚合指标
  `total_orders,scheduled_orders,blocked_orders,total_fulfilled_qty,quota_utilization`

### substitution

- `substitution_allocations.csv`
  `day,parent_part_id,alt_part_id,allocated_qty,day_allocated,alt_class`
- `substitution_decisions.csv`
  `category,chosen_part_id,basis,net_demand,remaining_on_hand_after`
- `substitution_water.csv`
  `member_part_id,target_ratio,historical_qty,on_hand_before,safety_stock,allocatable_before,on_hand_after`

## 约束键约定（重要）

四维约束键 `day / family_id / cust_group_id / region_id` **只在 C++ CLI 侧计算**
并写入 CSV，Node 侧不得重算：

- `family_id     = part_id / 10`
- `cust_group_id = std::hash<std::string>()(customer_group) % 100`
- `region_id     = std::hash<std::string>()(region) % 10`

`std::hash` 属实现定义行为（不同标准库结果可能不同），因此键必须由产出它的
同一二进制输出，跨进程复用会导致不一致。

## 退出码

| 码 | 含义 |
| :--- | :--- |
| 0 | 成功 |
| 2 | 参数错误（缺少 `--engine/--in/--out`、未知参数、未知引擎类型） |
| 3 | 输入数据缺失或不可读 |
| 4 | 计算或写出失败（含替代料组不存在等业务数据问题） |

## 示例

```bash
# 交付承诺：需求 30 件 @ Day3 -> 承诺 Day 3，预留 3 工时
mkdir -p /tmp/out
./bin/ipc_engine_cli --engine delivery \
    --in data/demo/delivery-benchmark --out /tmp/out \
    --part-id 0 --due-day 3 --qty 30
cat /tmp/out/delivery_result.csv

# ITP 主计划配额
./bin/ipc_engine_cli --engine itp --in data/demo/itp-iop-benchmark --out /tmp/out

# IOP 刚性阻断（下派 3 / 阻断 1）
./bin/ipc_engine_cli --engine iop --in data/demo/itp-iop-benchmark --out /tmp/out

# 替代料三类决策（分配 10，安全库存 10 未侵占）
./bin/ipc_engine_cli --engine substitution \
    --in data/demo/substitution-benchmark --out /tmp/out \
    --alt-class 3 --alt-group 3 --net-demand 12 --day 1
```

## 回归

```bash
./cli/cli_regression.sh        # 覆盖四引擎的表头/行数/关键数值断言
```
