# Spec Delta

## Purpose

提供面向业务与评审者的可视化界面，基于 DuckDB 中的数据，把 IPC 引擎的场景输入、决策过程与输出以图表形式呈现，覆盖场景浏览、计划透视、交付承诺、ITP/IOP 协同、替代料决策与性能对比。

## ADDED Requirements

### Requirement: Scenario browser

界面 SHALL 提供场景浏览能力，使用户可按测试场景查看该场景的全部输入数据与输出数据。

#### Scenario: List and select scenarios

- **WHEN** 用户打开界面
- **THEN** 界面列出可用测试场景及其数据集概览
- **THEN** 用户选择某场景后，界面各视图切换为该场景的数据

#### Scenario: Browse input data by business category

- **WHEN** 用户浏览某场景的输入数据
- **THEN** 界面按需求、供给、资源、计划供给、物料主数据、产品结构与替代料组分类展示，使这些信息可在同一处统一查看
- **THEN** 每类数据以表格形式呈现，并标明行数与来源数据集

#### Scenario: Inspect raw CSV content

- **WHEN** 用户选择查看某数据集的原始内容
- **THEN** 界面展示该 CSV 的原始文本，使用户可核对导入前的数据

#### Scenario: Browse run output data

- **WHEN** 用户选择某场景的一次运行
- **THEN** 界面展示该次运行的输出数据，并可切换该场景下的其他运行

#### Scenario: Indicate missing datasets

- **WHEN** 某场景缺少某类数据集，或某引擎在该场景下不可运行
- **THEN** 界面明确标出缺失与不可运行的引擎，而非展示空白表格

### Requirement: Planning pivot view

界面 SHALL 提供计划透视图，以类似 SAP IBP 计划视图的形式呈现关键指标在天粒度上的计划演进。

#### Scenario: Render pivot grid

- **WHEN** 用户打开计划透视图
- **THEN** 界面以「行 = 关键指标，列 = 天」的网格形式呈现，时间桶按天连续排列
- **THEN** 用户可横向滚动查看完整时间范围

#### Scenario: Group key figures by family

- **WHEN** 计划透视表被呈现
- **THEN** 关键指标按供给、需求、资源与平衡四类分组，使用户可折叠或展开各组

#### Scenario: Filter pivot

- **WHEN** 用户按物料、工作中心、客户组或大区筛选
- **THEN** 界面仅展示匹配的行，并按天重算取值

#### Scenario: Distinguish blank from zero

- **WHEN** 某指标在某天无计划
- **THEN** 界面以空白单元格呈现，与数值 0 区分

#### Scenario: Highlight overload and shortage

- **WHEN** 某天的剩余产能为负，或某天存在需求缺口
- **THEN** 界面以视觉强调标出该单元格，使用户能定位超载与缺口发生的时间点

#### Scenario: Drill down from a cell

- **WHEN** 用户点击某指标在某天的单元格
- **THEN** 界面展示构成该值的底层记录明细

### Requirement: Delivery promising visualization

界面 SHALL 提供交付承诺视图，展示 BOM 展开树、库存与产能水位、承诺日时间轴，以及回滚发生时的行为。

#### Scenario: Display fulfillable result

- **WHEN** 一次交付承诺计算返回可承诺结果
- **THEN** 界面在时间轴上标出请求交期与承诺交期，并以可视化方式区分二者先后关系
- **THEN** 界面展示各层 BOM 节点的需求量与供给来源

#### Scenario: Display blocked result

- **WHEN** 一次交付承诺计算返回不可承诺结果
- **THEN** 界面以显著方式标明阻塞，并展示未满足的数量缺口

#### Scenario: Visualize rollback

- **WHEN** 响应中包含大于零的回滚步数
- **THEN** 界面展示回滚步数并标识发生回滚的 BOM 层级

### Requirement: ITP/IOP alignment visualization

界面 SHALL 提供 ITP/IOP 协同视图，按四维约束键展示主计划配额与执行消耗，并标出被刚性阻断的订单。

#### Scenario: Show quota versus consumption

- **WHEN** 一次协同运行结果被加载
- **THEN** 界面为每个约束键展示配额上限与已消耗量，并对接近或达到上限的键加以区分

#### Scenario: Highlight blocked orders

- **WHEN** 运行结果包含被阻断的订单
- **THEN** 界面列出这些订单及其约束键，并说明其突破配额

#### Scenario: Show alignment metrics

- **WHEN** 运行结果被加载
- **THEN** 界面展示总订单数、下派数、阻断数与配额消耗率

### Requirement: Substitution decision visualization

界面 SHALL 提供替代料决策视图，展示一类、二类、三类替代料的分组水位、配额比例与分配结果。

#### Scenario: Show class comparison

- **WHEN** 替代料决策结果被加载
- **THEN** 界面按类别分组展示成员的目标配额比例、历史分配量与本次分配量

#### Scenario: Show safety stock boundary

- **WHEN** 展示三类替代料的可动用水位
- **THEN** 界面以可视化方式区分安全库存保护带与可分配部分

#### Scenario: Show selected alternative

- **WHEN** 某次决策选中了具体替代料
- **THEN** 界面高亮被选中的成员并显示选择依据（偏差值或优先级）

### Requirement: Performance and run history visualization

界面 SHALL 提供性能视图，展示单次运行耗时、吞吐量与历史运行对比。

#### Scenario: Show single run performance

- **WHEN** 一次引擎运行完成
- **THEN** 界面展示该次运行的耗时与处理条数，并在可计算时展示吞吐量

#### Scenario: Compare historical runs

- **WHEN** 用户查看某一引擎类型的历史运行
- **THEN** 界面以图表形式对比各次运行的关键指标

#### Scenario: Inspect a historical run

- **WHEN** 用户从历史列表中选择一次运行
- **THEN** 界面加载并展示该 `run_id` 对应视图，无需重新执行计算

### Requirement: Step-by-step replay

界面 SHALL 支持对决策过程的单步前进、单步后退与自动播放回放。

#### Scenario: Step through decisions

- **WHEN** 用户对一次运行使用单步前进
- **THEN** 界面按决策顺序推进一个步骤，并更新对应可视化状态

#### Scenario: Auto play

- **WHEN** 用户启动自动播放
- **THEN** 界面按固定节奏自动推进步骤直至结束
- **THEN** 用户可随时暂停或重置回放

### Requirement: Scenario input and execution from UI

界面 SHALL 允许用户调整场景输入参数并触发引擎运行，运行结果应在同一界面中呈现。界面 SHALL 仅通过后端 API 读取经 DuckDB 持久化的结果，不直接访问 C++ 可执行程序或数据文件。

#### Scenario: Adjust and run

- **WHEN** 用户修改场景参数并提交运行
- **THEN** 界面展示运行中的状态，并在完成后渲染结果视图

#### Scenario: Surface engine orchestration failure

- **WHEN** 后端报告 C++ 引擎执行失败、超时或可执行文件缺失
- **THEN** 界面呈现可读的失败原因，并保留用户已输入的场景参数以便重试

#### Scenario: Show validation errors

- **WHEN** 用户提交的参数被服务端拒绝
- **THEN** 界面呈现可读的校验错误，并保持已输入的场景参数不丢失
