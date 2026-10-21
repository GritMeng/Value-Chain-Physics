# IPC 智能计划控制塔企业级部署与容器化规范 (Enterprise Deployment & Containerization Specification)

本规范详述了 **IPC 智能计划协同工作台** 的分发打包、本地一键部署以及基于 Docker/Docker Compose 的跨平台容器化部署方案。本方案完美隔离并集成了：
- **前端网页**：React 19 + Tailwind v4 + Vite 的现代编译产物（`frontend/dist`）。
- **后台服务**：Python FastAPI + Uvicorn 极速异步网关（`server.py`）。
- **物理数据层**：DuckDB 超高性能分析型时序列存数据库（`ipc.db`）。
- **计算核心**：C++ 裸金属面向数据设计 (DOD) 有限产能有限物料求解引擎（`main_mem3.exe` / `test_runner_validation.exe`）。

---

## 🕸️ 部署架构拓扑 (Deployment Topology)

```mermaid
graph TD
    Client[浏览器/移动端客户端] <-->|HTTP / WebSocket | Gateway[FastAPI Uvicorn Gateway: 8501]
    
    subgraph Web App Container (FastAPI Server)
        Gateway <-->|Static Files Mount| Static[React 19 App: frontend/dist]
        Gateway <-->|DuckDB Connector| DB[(DuckDB Plan Database: ipc.db)]
        Gateway <-->|Process Spawning| Engine[C++ DOD MRP/Coproduct Solver]
    end

    DB <-->|Physical File Mapping| Engine
```

---

## ⚙️ 环境配置与环境变量 (Environment Configurations)

系统支持通过环境变量进行个性化微调，默认参数如下：

| 环境变量 (Env Var) | 默认值 (Default) | 描述 (Description) |
| :--- | :--- | :--- |
| `PORT` | `8501` | 控制塔网关监听端口 |
| `DB_PATH` | `ipc.db` | 物理 DuckDB 文件位置。沙箱启用时会自动克隆为 `sandbox_<scenario_code>.db` |
| `ACTIVE_SCENARIO` | `baseline` | 系统初始化默认物理计划分支（Baseline 生产主计划） |

---

## 윈도우 Windows 企业级一键安装分发 (Windows Deployment)

Windows 工作站与物理服务器是 IPC 传统求解的基准环境，系统内置了**分发打包器**与**一键向导安装程序**。

### 1. 打包分发文件 (Packaging)
在开发工作站的根目录执行以下命令：
```cmd
deploy_ipc_engine.bat
```
该脚本将自动清空并初始化 `dist/ipc_platform` 文件夹，复制 C++ 求解器可执行程序、FastAPI 后台、现代 React 19 编译静态包 (`frontend/dist`)、DuckDB 空白数据库模板，并自动注入部署引导说明。

### 2. 目标服务器安装 (Installation)
将打包生成的 `dist/ipc_platform` 文件夹压缩打包并发送至目标客户服务器，右键以 **管理员身份运行** `install.bat` 脚本：
- **路径自选**：可输入自定义安装目录（如 `C:\IPC_Platform`）。
- **环境自愈**：自动检测 Python 3.8+ 并静默安装依赖包：`fastapi uvicorn duckdb pandas`。
- **自动创建快捷方式**：在管理员桌面生成 `IPC_Control_Tower.lnk`。双击即可后台拉起 uvicorn 实例并开启浏览器访问 `http://127.0.0.1:8501`。

---

## 🐋 跨平台 Docker 容器化部署 (Docker Containerization)

为了支持华为云、腾讯云、美团云或私有 K8s 容器云的微服务部署，我们提供了基于 Linux 镜像的多阶段构建编译容器化方案。

### 1. Dockerfile 配置
在项目根目录下创建 `Dockerfile`（支持多阶段构建以保持最终镜像的轻量化，并在编译期拉起 Linux `g++` 编译 C++ 求解器内核）：

```dockerfile
# ==========================================
# 阶段一：多语言编译环境 (Node + GCC)
# ==========================================
FROM python:3.11-slim AS builder

# 安装 C++ 引擎编译所需的编译工具链和 curl
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    curl \
    && rm -rf /var/lib/apt/lists/*

# 安装 Node.js 用于编译 React 19 前端
RUN curl -fsSL https://deb.nodesource.com/setup_18.x | bash - \
    && apt-get install -y nodejs

WORKDIR /app

# 1. 编译前端静态代码
COPY frontend/package*.json ./frontend/
RUN cd frontend && npm install

COPY frontend/ ./frontend/
RUN cd frontend && npm run build

# 2. 编译 C++ 求解器内核 (Linux 裸金属可执行程序)
COPY *.cpp *.h ./
# 提示：Linux 下编译需安装 libduckdb 动态链接库或使用 DuckDB Amalgamation
# 目标：生成 Linux 环境下的 main_mem3 二进制求解器
RUN g++ -O3 -std=c++17 main.cpp csv_export.cpp -o main_mem3 -lduckdb

# ==========================================
# 阶段二：生产运行环境 (Python Light Image)
# ==========================================
FROM python:3.11-slim

WORKDIR /app

# 从第一阶段复制 Node React 编译包与 C++ 二进制包
COPY --from=builder /app/main_mem3 ./
COPY --from=builder /app/frontend/dist ./frontend/dist
COPY server.py ipc.db ./

# 创建空白模板文件夹防止后端挂载报错
RUN mkdir templates

# 安装 Python 运行依赖
RUN pip install --no-cache-dir fastapi uvicorn duckdb pandas

EXPOSE 8501

ENV DB_PATH=ipc.db
ENV PORT=8501

# 启动 uvicorn 网关
CMD ["python", "-m", "uvicorn", "server:app", "--host", "0.0.0.0", "--port", "8501"]
```

### 2. Docker Compose 微服务编排
通过创建 `docker-compose.yml` 实现快速一键拉起：

```yaml
version: '3.8'

services:
  ipc-control-tower:
    build:
      context: .
      dockerfile: Dockerfile
    image: ipc-control-tower:latest
    container_name: ipc_control_tower_service
    ports:
      - "8501:8501"
    volumes:
      # 将数据库存盘挂载到宿主机，实现沙箱与生产计划的持久化
      - ./data:/app/data
    environment:
      - DB_PATH=/app/data/ipc.db
      - PORT=8501
    restart: always
    logging:
      driver: "json-file"
      options:
        max-size: "10m"
        max-file: "3"
```

通过以下指令即可拉起容器：
```bash
docker-compose up -d --build
```

---

## 🚨 高可用与部署故障诊断 (Troubleshooting)

### 1. Mixed Encoding 编码异常
- **表现**：启动 Python 服务时抛出 `SyntaxError: Non-UTF-8 code starting with '\x98'`。
- **原因**：以往在 Windows 下修改的文件混入 GBK 编码。
- **解决**：确保所有的 python 源码首行统一使用 `# -*- coding: utf-8 -*-` 并转为 UTF-8 无 BOM 格式。

### 2. 端口占用限制
- **表现**：端口 8501 报错 `[Errno 98] Address already in use`。
- **解决**：
  - Windows: 执行 `taskkill /f /im python.exe` 或 `netstat -ano | findstr 8501` 获取 PID 后强行终止。
  - Linux: 执行 `fuser -k 8501/tcp` 释放端口。
