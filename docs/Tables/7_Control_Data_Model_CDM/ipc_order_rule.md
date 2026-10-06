---
table_name: "ipc_order_rule"
alias: "order_rule"
module: "7_Control_Data_Model_CDM"
cpp_struct: "None"
status: "Enriched"
tags:
  - ipc-table
  - enriched
  - 7_control_data_model_cdm
---

# 🏷️ `ipc_order_rule` (order_rule)

> **业务说明**: 确定计划订单的规则和日期

| 字段编码 (Field Code) | 字段物理名 (Field Name) | 物理类型 (Data Type) | 必填/键 (Key) | 详细业务备注 (Comment) |
| :--- | :--- | :--- | :--- | :--- |
| `last_lot_size` | last_lot_size | `BOOLEAN` | Nullable | 对于每个计划订单规则，指定是否将批量规则应用于计划区间horizon的最后一个计划订单。批量规则包括最小订单数量,最大订单数量,和订单倍数。
Y - 启用
N - 不启用 |
| `max_usage` | max_usage | `VARCHAR(10)` | Nullable | average - 如果所需数量超过part_source.max_qty，使用max_qty来确定要生成的计划订单数量，然后对这些订单的所需数量求平均值。
此选项允许只覆盖给定的需求，尊重任何最小、最大或倍数策略，而不会在必要之前引入任何供应。为了使用这个选项，给定的需求必须只有一个可以满足它的part_source，并且值
part_source.tak_time必须设置为0
campaign - 每个已规划的订单都是按照“零件来源.批量大小”中指定的精确数量（批次大小）来创建的。此设置会在生产计划中按照每个计划的最小和最大数量进行供应安排，这些最小和最大数量由“零件来源”记录中的“最小数量”和“最大数量”字段决定。因此，每个计划的最小批次数量计算为“最小数量”除以“批量大小”，而每个计划的最大批次数量计算为“最大数量”除以“批量大小”。
此外，如果零件来源受到限制，则给定计划中的所有批次都必须通过限制的连续生产进行安排。
此选项支持可选的计划规划功能。
ignore - 忽略
use - 应用 |
| `description` | description | `VARCHAR` | Nullable | 计划单据产生规则的描述 |
| `min_usage` | min_usage | `VARCHAR(10)` | Nullable | ignore - 忽略
use - 应用 |
| `multi_usage` | multi_usage | `VARCHAR(10)` | Nullable | ignore - 忽略
use - 应用 |
| `order_gen_rule` | order_gen_rule | `VARCHAR(10)` | Nullable | 控制如何产生计划单据
no_order - 不产生计划单据
any_time - 任何时间都能产生计划单据，过期也可以
run_date - 运行之日之后
after_ptf - ptf 之后产生 |
| `ptf_rule` | ptf_rule | `VARCHAR(10)` | Nullable | 说明了对于采用此订单策略的零部件来源，其规划时间界限日期（即 PartSource.PTFDate）的计算方式。
通常情况下，规划时间界限可能设定为零部件的交货期或指定的工作日数。如果“OrderGenerationRule”被设置为“AfterPTF”，那么计算出的 PTFDate 就代表了允许该零部件来源的新计划订单最早到期的日期。
fence - rundate + part.planning_time_fence, ptf unit要看取物料还是取source.
last_due_fence - fence或者last SR, 计划订单的due date最晚日期
last_due_lead - lead 或者due date的最晚日期
lead - lead time + time fence+run date

 |
| `ptf_unit` | prf_unit | `VARCHAR(10)` | Nullable | 时间单位如何选择
part - 从part数据取值
source - 从source数据取值 |
| `order_rule` | order_rule | `VARCHAR(10)` | PK / NOT NULL | order_rule id |
| `yield_usage` | yield_usage | `VARCHAR(10)` | Nullable | 说明了如何在物料级别上解读产量值。这
该设置会针对“partsource.yield”字段（以考虑废料因素）以及“partsource.coproductyield”字段（以考虑副产品和副产物流量）进行计算。
对于计划订单，会根据需求所要求的数量来应用产量，从而生成“plannedorder”。数量
（为保证产量所需的数量，其中计入库存的净数量在“planned_order.eff_qty“中予以报告。）
ignore - 忽略
yield_fraction - 从0到1， 1意味着no loss.
yield_percent - 从1到100， 100意味着没有损失
scrap_fraction - 从0到1， 0意味着no loss
scrap_percent - 从1到100， 0意味着no loss
scrap_fixed - 每个单据都是损失一个固定的数量
inflation_fraction - 1/(1+part_source.yield)
inflation_percent - 1/(1+part_source.yield*0.01)
 |

##### 🔄 算法编排与数据结构设计 (Algorithm & Data Structure Specification)

###### 1. 业务算法编排：计划订单生成约束与 PTF 时间屏障
* **因果流向**：`ipc_order_rule` 表配置了生成计划补货工单时的约束边界。
* **算法编排**：
  1. PTF 时间屏障拦截（Planning Time Fence）：引擎计算零部件的 PTFDate。若 `order_gen_rule = 'AfterPTF'`，系统禁止在 PTFDate（通常是制造提前期）之前生成任何新的生产工单，仅允许在时间屏障之后创建供应，用以保证近期车间计划的稳定，防范频繁重算导致车间频繁换产。
  2. 批量规则处理：若需求数量为 $Q$，根据 `min_usage`, `max_usage` 从 `part_source` 抓取最小起订量 $Min$、最大订单限制 $Max$ 和倍数 $Multiple$。生成的工单数量 $Q_{final}$ 满足：
     $$ Q_{final} = \max(Min, \lceil \frac{Q}{Multiple} \rceil \times Multiple) $$