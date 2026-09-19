# Spec Delta

## Purpose

将场景的输入数据集 CSV 与每次运行的输出 CSV 一并导入 DuckDB，使用户可在统一界面查看需求、供给、资源与计划供给等全部信息，并支持按运行批次回溯。

## ADDED Requirements

### Requirement: Input dataset ingestion into DuckDB

系统 SHALL 将场景目录中的七类输入数据集导入 DuckDB，分别为物料主数据、BOM、产能、ATP 供给、主计划需求、执行计划需求与替代料组。导入 SHALL 保留与 C++ 加载器一致的列契约，使同一份数据既能被用户查询浏览，也能被回写为引擎输入 CSV。

#### Scenario: Import a scenario's input datasets

- **WHEN** 用户触发某场景的数据导入
- **THEN** 系统读取该场景目录下的全部数据集 CSV，在 DuckDB 中建立对应表并写入全部行
- **THEN** 每行数据可关联到其来源场景，使不同场景的数据互不混淆

#### Scenario: Import reports row counts

- **WHEN** 一次场景导入完成
- **THEN** 系统报告每类数据集写入的行数与导入耗时

#### Scenario: Re-import is idempotent

- **WHEN** 用户对同一场景的同一数据集重复执行导入
- **THEN** 该数据集在导入后的行数与首次导入一致，不出现重复记录

#### Scenario: Multiple scenarios coexist

- **WHEN** 多个场景的数据集均被导入
- **THEN** 用户可按场景筛选查询任一类数据集，且各场景行数与其源文件一致

#### Scenario: Full dataset import is optional

- **WHEN** 用户选择导入规模较大的场景数据集
- **THEN** 系统完成导入并报告导入耗时与总行数
- **THEN** 小规模场景的导入路径不因此被强制加载大数据

### Requirement: Input data is queryable by business category

系统 SHALL 使用户可按业务类别查询输入数据，覆盖需求、供给、资源与计划供给。

#### Scenario: View demand data

- **WHEN** 用户查询某场景的需求信息
- **THEN** 系统返回主计划需求与执行计划需求，含物料、交期、数量、优先级、客户组与大区

#### Scenario: View supply data

- **WHEN** 用户查询某场景的供给信息
- **THEN** 系统返回 ATP 供给节点，含供给类型（在手/在途/计划订单）、可用日与数量

#### Scenario: View resource data

- **WHEN** 用户查询某场景的资源信息
- **THEN** 系统返回按工作中心与天组织的产能工时记录

#### Scenario: View planned supply data

- **WHEN** 用户查询某场景的计划供给信息
- **THEN** 系统返回供给记录中类型为计划订单的节点，并可按可用日排列

#### Scenario: View material and structure master data

- **WHEN** 用户查询某场景的物料与产品结构
- **THEN** 系统返回物料主数据（安全库存、期初在手、批量、提前期）与 BOM 结构（含替代料等级、目标比例、历史量）

### Requirement: Run artifact ingestion into DuckDB

系统 SHALL 为每次运行分配唯一 `run_id`，并将该次运行的场景标识、输入参数与 C++ 引擎产出的输出 CSV 导入 DuckDB。导入结果 SHALL 足以在不重新调用 C++ 引擎的情况下重建该次运行的结论与可视化数据。

#### Scenario: Ingest delivery promising artifacts

- **WHEN** 一次交付承诺运行产出了输出 CSV
- **THEN** 系统写入一条运行记录，包含 `run_id`、场景标识、引擎类型、创建时间、输入参数、退出码与耗时
- **THEN** 输出 CSV 中的结果与步骤明细被导入结构化结果表，并可通过 `run_id` 关联查询

#### Scenario: Ingest ITP/IOP alignment artifacts

- **WHEN** 一次 ITP/IOP 协同运行产出了输出 CSV
- **THEN** 系统导入每个四维约束键（day、family、cust、region）的配额与消耗明细，以及逐条执行需求的下派或阻断状态
- **THEN** 被阻断的订单记录包含其约束键与阻断原因

#### Scenario: Ingest substitution artifacts

- **WHEN** 一次替代料决策运行产出了输出 CSV
- **THEN** 系统导入每次分配记录，包含替代料类别、父物料、被选中的替代料、分配数量与分配日

#### Scenario: Preserve raw artifacts

- **WHEN** 任意一次运行完成
- **THEN** 该次运行的输入与输出 CSV 原始内容被保留，可被检索用于审计或重新导入

### Requirement: Run history query and retrieval

系统 SHALL 提供按场景、运行类型与时间检索历史运行的能力，并支持按 `run_id` 取回完整结果。

#### Scenario: List recent runs

- **WHEN** 用户请求某一场景或某一引擎类型的最近运行列表
- **THEN** 系统返回按创建时间倒序排列的运行摘要，至少包含 `run_id`、场景标识、运行类型、创建时间与关键结论

#### Scenario: Retrieve a run by id

- **WHEN** 用户以存在的 `run_id` 请求运行详情
- **THEN** 系统返回该次运行的完整输入参数与结果

#### Scenario: Unknown run id

- **WHEN** 用户请求一个不存在的 `run_id`
- **THEN** 系统返回明确的未找到错误，且不返回空白的成功响应

### Requirement: Reproducibility and provenance of persisted data

系统 SHALL 记录每类数据的来源标识，使输入数据与历史运行均可被追溯其来源。

#### Scenario: Trace input data provenance

- **WHEN** 用户查询某场景输入数据的元信息
- **THEN** 响应中包含该场景标识、源目录位置与最近一次导入时间

#### Scenario: Trace run provenance

- **WHEN** 用户查询一次历史运行的详情
- **THEN** 响应中包含该次运行使用的场景标识与数据集导入时间
