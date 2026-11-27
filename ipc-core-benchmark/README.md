# IPC Core Open Benchmarks v2.2：无菌决策质量与算力开源验证套件

> **零依赖、可审计、洁净室原生的 C++17 核心算法微内核与七大可复现验证套件（附 Python 独立交叉对账与九大免安装网页交互沙盘）**

[![License](https://img.shields.io/badge/License-Non--Commercial_Reproducibility_%7C_No_Patent_Grant-b03a2e.svg)](../LICENSE)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-2ea043.svg)](https://en.cppreference.com/w/cpp/17)
[![Interactive Demo](https://img.shields.io/badge/Online_Sandbox-9_Live_Scenarios-0969da.svg)](https://gritmeng.github.io/Value-Chain-Physics/demo/)

---

> [!IMPORTANT]
> **关于开源边界、知识产权保护与验证目的的正式声明**
> 1. **无需安装也可在线测**：不熟悉 C++ 编译的管理者、学者与同行，可直接打开 [**免安装浏览器九大交互沙盘 (`demo/index.html`)**](https://gritmeng.github.io/Value-Chain-Physics/demo/)，自由修改库存、交期、配额、产能、规格档位与国别认证参数，实时对比现有 SCP / APS 死锁与 IPC 正交计算结果。
> 2. **严格的商业机密与专利隔离边界**：本目录仅公开脱敏后的最小完备数学模型与 C++17 可审计微内核，**不包含**生产级 IPC 商业引擎底层内存数据结构、完整企业数据本体、并发沙箱源码或企业部署平台。作者名下 **20 项发明专利（5 项已获授权，15 项在审/公开）100% 保留**（详见仓库根目录 [`LICENSE`](../LICENSE)）。
> 3. **开源验证目的**：向全球学者、审稿人、架构师与制造业同行证明，IPC 的核心决策质量（排序耦合解耦、组替代盘活、CODP 两层定位、零预测冲减三态路由、先齐套再切瓶颈配额与逆向重展防超买、跨 CODP 两阶段推拉排产、价值链网络先序区间与维度规划）与大规模线性/准线性算力响应在数学与工程上 100% 可复现、可审计。

---

## 七大可复现验证套件总览 (7 Auditable C++17 & Python Verification Suites)

| 序号 | 可执行测试目标 | 验证的核心原创机理 | 关键确定性断言结果 |
| :---: | :--- | :--- | :--- |
| **1** | `test_delivery_precision` | **《排序耦合定律》（2 需求 × 3 料 × 4 周期）+ 多阶 ATP 全树零残留原子回滚** | 传统静态锁库致 $D_2$ 延期 1 天、现货闲置 3 天；IPC 两阶段解耦双双准时（0 延期）；缺料查询全树零残留回滚 |
| **2** | `test_substitution_rules` | **成套组替代（`¥11,000` 长板 vs `¥1,000` 短板拉快）+ 不完全替代（主料锚定 `30%` 红线 & 单向属性门槛）+ Class 1/3 配额重归一化** | 总成本 `¥23,000 -> ¥12,000`，净省现金 `¥11,000/套` 且呆滞归零；主料缺料至 `500` 时现有系统副料占比飙至 `37.5%` 违约，IPC 精准截断 `214` 颗（`29.97%`）；`1,000` 颗按 `MPQ=300` 精准收敛 `600+300+100` |
| **3** | `test_codp_positioning` | **推拉解耦点（CODP）两层非补偿定位 + 二阶修宪 $\Phi$ 移点影子收益（3 层 6 节点）** | Gate 1 一票淘汰 42d 芯片；沙漏腰部裸机池使安全库存台数直降 `50.0%`（`640 -> 320`）、资金直降 `62.5%`（`¥51.2万 -> ¥19.2万`）；旧工艺早镭雕自动弹出 `¥32.0万` 影子收益 |
| **4** | `test_push_pull_router` | **废除孤立预测冲减：推式防首日独占日切割 + 拉式超额 `Mode A / B / C` 三态路由** | `W1 800k` 精确平准切割至 `D1..D6`（`480k + 320k` 守恒）；节奏前移 `Mode A` 零虚买、真增量 `Mode B/C` 精准分流 |
| **5** | `test_itp_iop_alignment` | **ITP 齐套先行与最紧瓶颈（`Most-Gating`）战略保底+公平配额切分、逆向重展防超买及 IOP 独立配额桶阻断** | `6,000` 颗共用芯片在齐套封顶（`5,000 + 3,000`）后按战略保底 `2,000` 与公平对半切分为 `4,000 + 2,000`，逆向重展消除 `3,000` 颗非瓶颈件超买呆滞，执行期拦截越界单并保住常规区 `100%` 交付 |
| **6** | `test_two_stage_iop_network` | **跨 CODP 两阶段受限推拉排产（`Push-IOP + Pull-IOP`）+ 价值链网络先序区间 `[rank..end]` $O(1)$ 子树代数、维度规划（`GE/EQ`）与国别认证位掩码剪枝、菱形 DAG 去重守恒及逆向 `Where-Used` 反查** | 传统按后道 1 天静态提前期盲目承诺致 `550` 台爽约违约，IPC 两阶段联动诚实承诺 `EU=D4, AF=D5, LATAM=D6`（`0` 爽约、拦截越界急单 `150` 台）；先序区间 `[1..7]` $O(1)$ 包含判定 + 菱形共用件流量 `100+200=300` 精确守恒 + 逆向精准反查双顶层成品 |
| **7** | `stress_benchmark_2m` | **500,000 并发需求 × 2,000,000 物料节点 × 20 层价值链 DAG 网络极限压测 + 1,000,000 次 $O(1)$ 先序区间子树查询** | 真实构建 `L0..L19` 20 层 `425 万` 跨层共享 BOM 边：Kahn 拓扑排序 + 20 层级联净需求消纳 + 零残留守恒对账 + 50 万需求 ITP/IOP 防波堤 + 200 万节点先序区间构建与 100 万次毫秒级子树归属判定 |

---

## 一键本地编译与双发对账 (Quick Start: Zero External Dependencies)

只需标准 C++17 编译器（MSVC / GCC / Clang）与 Python 3，**无需安装任何数据库或第三方库**。

### Windows 一键编译并运行全部 7 组套件 + Python 10 项交叉对账

```cmd
.\compile_and_run.bat
```

### Linux / macOS / 跨平台 CMake 编译与验证

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

./build/test_delivery_precision
./build/test_substitution_rules
./build/test_codp_positioning
./build/test_push_pull_router
./build/test_itp_iop_alignment
./build/test_two_stage_iop_network
./build/stress_benchmark_2m

python3 verifier/cross_validator.py
```

详细数学方程与算例推导见 [`docs/MATHEMATICAL_SPEC.md`](docs/MATHEMATICAL_SPEC.md)。

> **性能测算口径透明说明**：`stress_benchmark_2m` 实测纯算法核心 `~260 ms`（另含 200 万节点先序区间构建与 100 万次 $O(1)$ 子树查询毫秒级耗时）为本仓洁净室纯内存 C++17 核心算子耗时（不含磁盘 I/O 与复杂排程约束）；作者在工业实证论文与公众号/期刊文章中提及的 `296 秒` 为含完整企业数据加载、15 万项不等式约束求解与全链路落盘的工业级端到端实测。

---

## 许可协议与专利权保留声明 (License & Patent Notice)

本目录公开代码适用仓库根目录 [**LICENSE**](../LICENSE) 第 2 条与第 3 条：
- **非商业学术复现与企业内部评估专用许可**：允许高校、审稿人、研究者及企业技术团队在**非生产环境**下自由下载、编译、跑数验证与基准复现；严禁任何形式的生产环境部署、商业集成、付费咨询交付、转售或再授权（代码按“原样 AS IS”提供）；
- **专利权完全保留（NO PATENT LICENSE）**：作者名下 **20 项已授权及在审发明专利 100% 保留**，不因本公开测试包授予任何明示或默示专利许可。商业授权联系：`gritmeng@outlook.com`。
