# Spec Delta

## Purpose

提供以天为时间粒度、行为关键指标、列为时间桶的计划透视数据能力，使用户能像 SAP IBP 计划视图那样跨时间分析供给、需求、资源与库存平衡。指标依 IPC 引擎的业务场景确定。

## ADDED Requirements

### Requirement: Time-phased pivot structure

系统 SHALL 提供计划透视数据，其结构为「行 = 关键指标 × 筛选维度，列 = 天粒度时间桶」。时间桶 SHALL 覆盖该场景数据所涉及的天数范围。

#### Scenario: Pivot rows are planning key figures

- **WHEN** 用户请求某场景的计划透视
- **THEN** 每一行对应一个关键指标（可带维度成员），每一列对应该场景时间范围内的一个天
- **THEN** 时间桶按天升序排列，且相邻桶为连续日期

#### Scenario: Pivot time range derives from data

- **WHEN** 场景数据中最早与最晚的日期被确定
- **THEN** 透视的时间范围覆盖该区间，并包含产能与需求所涉及的全部天

#### Scenario: Cells are addressable

- **WHEN** 用户查看某个指标在某个天的数值
- **THEN** 系统可返回该单元格的值，并标明其指标、维度成员与日期

#### Scenario: Missing data is explicit

- **WHEN** 某指标在某天没有任何数据
- **THEN** 该单元格表现为空值而非 0，使用户能区分「无计划」与「计划为零」

### Requirement: Supply key figures

系统 SHALL 在计划透视中提供供给类关键指标，反映供给随时间的到达与可用情况。

#### Scenario: On-hand supply by day

- **WHEN** 用户查看在手供给指标
- **THEN** 系统按可用日展示类型为在手的供给数量

#### Scenario: Scheduled receipts by day

- **WHEN** 用户查看在途供给指标
- **THEN** 系统按可用日展示类型为 SR 的供给数量

#### Scenario: Planned orders by day

- **WHEN** 用户查看计划供给指标
- **THEN** 系统按可用日展示类型为计划订单的供给数量

#### Scenario: Total planned available supply by day

- **WHEN** 用户查看计划可用量指标
- **THEN** 系统按天汇总该日及之前所有可用供给（在手、在途、计划订单）的累计可用量

### Requirement: Demand key figures

系统 SHALL 在计划透视中提供需求类关键指标，反映需求随时间的分布。

#### Scenario: Independent demand by day

- **WHEN** 用户查看独立需求指标
- **THEN** 系统按交期展示主计划需求数量

#### Scenario: Execution demand by day

- **WHEN** 用户查看执行需求指标
- **THEN** 系统按交期展示执行计划需求数量

#### Scenario: Demand separated by planning stage

- **WHEN** 用户查看需求类指标
- **THEN** 主计划需求与执行计划需求分别成行，使用户可对比计划与执行的口径差异

#### Scenario: Gross demand by day

- **WHEN** 用户查看毛需求指标
- **THEN** 系统按天展示该日到期的需求总量，并可按场景中的需求来源加以区分

### Requirement: Resource key figures

系统 SHALL 在计划透视中提供资源类关键指标，反映产能随时间的占用与余量。

#### Scenario: Available capacity by day

- **WHEN** 用户查看产能指标
- **THEN** 系统按天与工作中心展示可用产能工时

#### Scenario: Consumed capacity by day

- **WHEN** 用户查看产能占用指标
- **THEN** 系统按天展示已被交付承诺预留的产能工时

#### Scenario: Remaining capacity by day

- **WHEN** 用户查看剩余产能指标
- **THEN** 系统按天展示可用工时扣除已分配工时后的余量
- **THEN** 当日占用超过可用工时时，剩余量表现为负值以暴露超载

### Requirement: Balance and commitment key figures

系统 SHALL 在计划透视中提供平衡类关键指标，反映承诺结果与供求缺口。

#### Scenario: Committed quantity by day

- **WHEN** 用户查看已承诺量指标
- **THEN** 系统按承诺日展示交付承诺运行中成功承诺的数量

#### Scenario: Quota versus consumption by day

- **WHEN** 用户查看配额类指标
- **THEN** 系统按天展示主计划防波堤配额与执行计划已消耗配额，使配额余量可由二者相减得出

#### Scenario: Blocked quantity by day

- **WHEN** 用户查看阻断类指标
- **THEN** 系统按天展示被刚性阻断的执行需求数量，使用户能看到越权插单发生的时间点

#### Scenario: Shortage and backlog by day

- **WHEN** 用户查看缺口类指标
- **THEN** 系统按天展示未能满足的需求量
- **THEN** 在存在跨天倚赖时，缺口与积压可被区分展示

### Requirement: Pivot filtering and drill-down

系统 SHALL 支持按场景与关键维度筛选计划透视，并支持从汇总单元格下钻到明细。

#### Scenario: Filter by scenario

- **WHEN** 用户选择一个场景
- **THEN** 透视仅展示该场景数据，且不混入其他场景的行

#### Scenario: Filter by key dimension

- **WHEN** 用户按物料、工作中心、客户组或大区筛选
- **THEN** 透视仅展示匹配的行，且各行的按天取值相应重算

#### Scenario: Drill down to detail

- **WHEN** 用户选择某个指标在某个天的单元格
- **THEN** 系统返回构成该值的底层记录（如具体的供给节点或需求单）

### Requirement: Pivot data consistency with engine results

计划透视 SHALL 与对应运行的引擎结果保持一致，不得出现与结果明细矛盾的指标值。

#### Scenario: Capacity consumption matches delivery runs

- **WHEN** 计划透视展示某场景的产能占用
- **THEN** 该值与交付承诺运行结果中的产能占用一致

#### Scenario: Quota and blocking match alignment runs

- **WHEN** 计划透视展示某场景的配额消耗与阻断量
- **THEN** 该值与 ITP/IOP 协同运行结果中的配额消耗与阻断计数一致

#### Scenario: Pivot is recomputed from persisted data

- **WHEN** 场景的输入数据或运行结果发生变化
- **THEN** 透视数据反映更新后的值，且无需前端自行推算指标
