# Spec Delta

## Purpose

以目录为单位定义、发现与管理测试场景，使每个场景自包含本次计算所需的全部数据集，并作为输入浏览、引擎运行与结果归档的统一组织维度。

## ADDED Requirements

### Requirement: Scenario as a self-contained directory

每个测试场景 SHALL 对应一个独立目录，内含该次计算所需的全部数据集。场景目录 SHALL 自包含，不依赖其他场景目录中的文件。

#### Scenario: Recognized scenario layout

- **WHEN** 系统扫描场景根目录
- **THEN** 每个含数据集的子目录被识别为一个场景，并以目录名作为场景标识
- **THEN** 场景根目录本身若含数据集，也作为一个场景被识别

#### Scenario: Scenario contains required datasets

- **WHEN** 用户查看一个场景
- **THEN** 该场景可提供物料主数据、BOM、产能、ATP 供给、主计划需求、执行计划需求与替代料组七类数据集
- **THEN** 每类数据集的内容来自该场景目录自身

#### Scenario: Scenario data stays within the scenario

- **WHEN** 某场景被运行或导入
- **THEN** 只读取该场景目录内的文件，不读取其他场景的数据集

### Requirement: Scenario discovery and listing

系统 SHALL 发现并列出可用场景，并为每个场景给出其名称、位置与数据集可用性概览。

#### Scenario: List available scenarios

- **WHEN** 用户请求场景列表
- **THEN** 系统返回按名称排列的场景列表，每项包含场景标识、显示名与数据集行数概览

#### Scenario: Existing datasets become initial scenarios

- **WHEN** 系统首次启动且场景根目录指向既有 `ipc-core-benchmark/data/`
- **THEN** `sample/` 与场景根目录本身均被识别为可用场景

#### Scenario: Empty scenario root

- **WHEN** 场景根目录不存在或不含任何数据集目录
- **THEN** 系统返回空场景列表并说明如何创建或导入场景，而非报错退出

### Requirement: Scenario integrity validation

系统 SHALL 校验场景数据集的完整性，并明确报告缺失或不可读的数据集。

#### Scenario: Report missing datasets

- **WHEN** 场景目录中缺少某类必需数据集
- **THEN** 系统标明缺失的数据集类别，并使需要该数据集的引擎在该场景下不可运行

#### Scenario: Report unreadable dataset

- **WHEN** 场景中的某数据集无法按预期列契约解析
- **THEN** 系统标明该数据集不可读及其原因，且不把部分解析结果当作完整数据

#### Scenario: Validation is reported per engine

- **WHEN** 用户查看场景详情
- **THEN** 系统按引擎类型分别标明该场景是否具备运行条件

### Requirement: Scenario creation

系统 SHALL 支持创建一个新的测试场景，使用户可在不影响既有场景的前提下组织自己的数据集。

#### Scenario: Create a new scenario

- **WHEN** 用户以名称创建一个新场景
- **THEN** 系统创建对应的场景目录，并使该场景出现在场景列表中

#### Scenario: Creation does not modify existing scenarios

- **WHEN** 新场景被创建或写入
- **THEN** 既有场景目录内容保持不变

#### Scenario: Duplicate scenario name rejected

- **WHEN** 用户以已存在的场景标识创建场景
- **THEN** 系统拒绝创建并返回明确的冲突说明

### Requirement: Scenario-scoped dataset upload or replacement

系统 SHALL 允许为场景提供或更新数据集文件，并使更新结果可被校验与浏览。

#### Scenario: Provide dataset for a scenario

- **WHEN** 用户为某场景提供一类数据集文件
- **THEN** 系统将其写入该场景目录，并重新校验该场景的完整性

#### Scenario: Replacement is reflected in views

- **WHEN** 某场景的数据集被更新
- **THEN** 该场景的输入浏览视图反映更新后的内容与行数
