# Spec Delta

## Purpose

定义演示应用的启动、配置、C++ 引擎构建与测试场景引导流程，使评审者可在本地以最少的步骤运行并复现文档中的基准结论。

## ADDED Requirements

### Requirement: One-command local startup

演示应用 SHALL 提供一条命令完成准备并启动前端与后端，准备步骤包括检查/构建 C++ 引擎可执行文件、创建 DuckDB 文件、发现场景并导入场景输入数据。

#### Scenario: First run from clean checkout

- **WHEN** 用户在 `ipc-demo/` 目录执行单一启动命令，且本地尚无 DuckDB 数据库文件与 C++ 可执行文件
- **THEN** 系统安装依赖、构建 C++ 引擎 CLI 可执行文件、创建 DuckDB 文件、发现测试场景并导入其输入数据，随后启动服务
- **THEN** 启动完成后输出可访问的本地地址

#### Scenario: Scenario root is configurable

- **WHEN** 用户通过配置指定自定义的场景根目录
- **THEN** 系统在该目录下发现测试场景，并在启动日志中列出发现的场景

#### Scenario: Subsequent startup

- **WHEN** 用户再次执行启动命令，且数据库已存在
- **THEN** 系统在不清空既有数据的前提下启动服务

### Requirement: Configuration via environment

演示应用 SHALL 通过环境变量或等价的本地配置文件暴露关键配置项，至少包含服务端口、DuckDB 文件路径、场景根目录、C++ 可执行文件路径与子进程运行时限。

#### Scenario: Override engine executable path

- **WHEN** 用户通过配置指定自定义的 C++ 可执行文件路径
- **THEN** 编排层调用该路径下的程序，并在健康检查中反映其是否存在

#### Scenario: Override database path

- **WHEN** 用户通过配置指定自定义的 DuckDB 文件路径
- **THEN** 应用在该路径读写数据，且不创建默认路径之外的数据库文件

#### Scenario: Override port

- **WHEN** 用户通过配置指定自定义端口
- **THEN** 应用在该端口监听，并在启动日志中输出实际使用的端口

### Requirement: Local-only default binding

演示应用 SHALL 默认仅绑定本机回环地址，避免在演示环境中意外对外暴露。

#### Scenario: Default binding

- **WHEN** 应用以默认配置启动
- **THEN** 服务仅接受来自本机回环地址的连接

### Requirement: Sample scenario presets

演示应用 SHALL 随仓库提供覆盖四个引擎的样例测试场景，每个场景为自包含目录且含可直接驱动 C++ CLI 的完整数据集，使每个可视化视图在首次启动后即有可展示的数据。

#### Scenario: Presets available after first run

- **WHEN** 用户首次打开界面
- **THEN** 场景列表中至少包含样例场景，且场景浏览、计划透视、交付承诺、ITP/IOP、替代料与性能各视图均具备可展示数据

#### Scenario: Preset reproduces documented conclusion

- **WHEN** 用户运行交付承诺样例场景中的需求 30 件 @ Day3
- **THEN** 结果复现基准测试中的结论：承诺 Day 3，并预留 3 工时

### Requirement: Documentation of usage and API

演示应用 SHALL 附带文档，说明启动方式、C++ 构建方式、测试场景的组织与创建方式、数据导入方式、配置项、C++ CLI 契约与后端 API 契约。

#### Scenario: Reader can run the demo

- **WHEN** 读者仅阅读演示应用文档
- **THEN** 文档提供从安装到打开界面的完整步骤，以及可选的样例/全量数据导入说明

#### Scenario: Reader can call the API

- **WHEN** 读者查阅文档中的 API 说明
- **THEN** 文档列出每个引擎端点的请求字段、响应字段与示例请求

#### Scenario: Reader can create a scenario

- **WHEN** 读者查阅文档中的场景组织说明
- **THEN** 文档说明场景目录应包含哪些数据集及其列契约，使读者可自行组织新的测试场景

#### Scenario: Reader can invoke the C++ CLI directly

- **WHEN** 读者查阅文档中的 C++ CLI 契约
- **THEN** 文档说明各引擎类型的输入 CSV、输出 CSV 路径约定与退出码语义，使其可脱离 Node 独立调用
