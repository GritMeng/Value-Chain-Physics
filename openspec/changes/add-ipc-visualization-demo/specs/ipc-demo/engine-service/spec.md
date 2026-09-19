# Spec Delta

## Purpose

由 Node.js 编排既有 C++ 引擎完成计算：Node 从测试场景写出输入 CSV、调用 C++ 可执行程序、读取其输出 CSV，并把结果交给持久化层；Node 侧不实现任何引擎算法。

## ADDED Requirements

### Requirement: C++ engine CSV-driven CLI contract

C++ 侧 SHALL 提供可通过命令行调用的引擎入口，使每一次计算都以「输入 CSV 目录 + 输出 CSV 目录」为边界完成。该入口 SHALL 复用既有引擎算子，且不得改变既有基准程序的行为。

场景目录 SHALL 可直接作为该入口的输入目录使用，使「测试场景」与「引擎输入」是同一个目录概念，无需额外格式转换。

#### Scenario: Run one engine over CSV inputs

- **WHEN** 以引擎类型、输入 CSV 目录与输出 CSV 目录调用 C++ 入口
- **THEN** 入口从输入目录读取该引擎所需的 CSV 文件，执行既有算子，并把结果写入输出目录的 CSV 文件
- **THEN** 进程以退出码 0 结束

#### Scenario: Scenario directory used directly as input

- **WHEN** 输入目录为一个测试场景目录
- **THEN** 入口能直接读取该场景内的数据集并完成计算，无需事先转换数据格式

#### Scenario: All four engines are individually invocable

- **WHEN** 分别以交付承诺、ITP、IOP、替代料四类引擎类型调用入口
- **THEN** 每类均可独立运行并产出各自的输出 CSV

#### Scenario: Existing benchmarks remain unaffected

- **WHEN** 新增 CLI 入口后运行既有基准程序
- **THEN** 既有 4 个基准程序的输出与断言结果保持不变

### Requirement: Engine input CSV provisioning

Node 后端 SHALL 为每次运行准备独立的输入 CSV，内容取自所选测试场景或请求中的显式参数，并符合 C++ 加载器所期望的列契约。C++ 入口 SHALL 以只读方式使用场景目录，后端不得在运行中改动场景目录内文件。

#### Scenario: Prepare inputs from selected scenario

- **WHEN** 一次运行基于所选测试场景发起
- **THEN** 后端为该次运行创建独立工作目录，并写出该引擎所需的全部输入 CSV
- **THEN** 写出的 CSV 表头与列顺序与既有 `data/*.csv` 契约一致

#### Scenario: Scenario directory is not mutated

- **WHEN** 一次运行结束
- **THEN** 所选场景目录中的文件内容与运行前一致

#### Scenario: Prepare inputs from explicit scenario parameters

- **WHEN** 请求携带覆盖场景的显式参数（如指定物料、交期、数量、优先级）
- **THEN** 后端把这些参数并入该次运行的输入 CSV，且不影响 DuckDB 中存放的基础数据集

#### Scenario: Concurrent runs do not interfere

- **WHEN** 两个运行同时发起
- **THEN** 各自使用相互隔离的工作目录，互不覆盖输入或输出文件

### Requirement: Engine subprocess invocation and result collection

Node 后端 SHALL 以子进程调用 C++ 可执行程序，并在其正常结束后读取输出 CSV 作为该次运行的权威结果。

#### Scenario: Successful invocation

- **WHEN** C++ 子进程以退出码 0 结束且输出 CSV 存在
- **THEN** 后端解析输出 CSV 得到该次运行的结果，并进入持久化流程

#### Scenario: Missing or unbuilt executable

- **WHEN** 配置的 C++ 可执行文件不存在或不可执行
- **THEN** 后端返回明确错误，指明可执行文件缺失，并给出构建指引
- **THEN** 该次请求不产生成功结果记录

#### Scenario: Non-zero exit code

- **WHEN** C++ 子进程以非零退出码结束
- **THEN** 后端将该次运行标记为失败，并在响应中包含退出码与捕获到的标准错误摘要
- **THEN** 不把不完整或缺失的输出 CSV 当作有效结果

#### Scenario: Execution timeout

- **WHEN** C++ 子进程超过配置的运行时限仍未结束
- **THEN** 后端终止该子进程，并把该次运行标记为超时失败
- **THEN** 响应中说明超时时限

#### Scenario: Malformed output CSV

- **WHEN** 输出 CSV 缺失必需列或无法解析为期望结构
- **THEN** 后端将运行标记为失败并说明输出格式问题，而非返回部分成功结果

### Requirement: Run provenance for engine invocation

后端 SHALL 为每次运行记录其场景来源、调用参数与产物位置，使运行可被审计与复现。

#### Scenario: Record invocation details

- **WHEN** 一次运行结束（无论成功或失败）
- **THEN** 记录中包含场景标识、引擎类型、可执行文件路径、命令行参数、输入与输出目录、退出码、耗时与失败原因（如适用）

#### Scenario: Retain artifacts for inspection

- **WHEN** 用户查看一次历史运行的详情
- **THEN** 可获取该次运行使用的输入 CSV 与产出的输出 CSV 的内容或归档位置

### Requirement: HTTP API for engine runs

后端 SHALL 通过 HTTP 以 JSON 暴露运行触发与结果查询能力，并区分非法请求、引擎失败与业务结论。

#### Scenario: Trigger a run

- **WHEN** 客户端以合法参数请求运行某一引擎
- **THEN** 后端完成编排与持久化后返回该次运行的 `run_id` 与结论摘要

#### Scenario: Invalid request rejected

- **WHEN** 请求缺少必填参数、数量为负数、引擎类型未知，或引用了不存在的物料
- **THEN** 后端返回客户端错误状态码与说明性信息，且不启动 C++ 子进程

#### Scenario: Engine failure distinct from business outcome

- **WHEN** 输入合法但引擎计算出「不可承诺」或「不可分配」
- **THEN** 后端返回成功状态码，并在响应体中表达该业务结论，且该次运行仍被归档

#### Scenario: Health check reflects prerequisites

- **WHEN** 客户端请求健康状态
- **THEN** 后端返回服务状态，并分别标明 DuckDB 连接是否就绪、数据集是否已导入、C++ 可执行文件是否存在
