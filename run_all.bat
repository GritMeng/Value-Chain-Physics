@echo off
title IPC Platform Auto Builder & Runner
color 0A

echo =====================================================================
echo          IPC 智能计划协同工作台 - 一键自动化编译与启动脚本
echo =====================================================================
echo.

:: 1. 检测 Node.js 并编译前端
echo [1/3] 正在检测 Node.js 运行环境...
node -v >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未检测到 Node.js，请先安装 Node.js！
    pause
    exit /b 1
)

echo [*] 正在编译 React + ag-Grid + Leaflet 前端静态资源 (npm run build)...
cd frontend
cmd.exe /c "npm run build"
if %errorlevel% neq 0 (
    echo.
    echo [错误] 前端编译打包失败，请检查报错！
    cd ..
    pause
    exit /b 1
)
cd ..

:: 2. 检测 Python 环境与依赖
echo.
echo [2/3] 正在检测 Python 环境...
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 未检测到 Python，请先安装 Python！
    pause
    exit /b 1
)

echo [*] 正在确保 Python 依赖包 (FastAPI, Uvicorn, DuckDB, Pandas) 已安装...
python -m pip install --quiet fastapi uvicorn duckdb pandas pydantic

:: 3. 启动后台服务器
echo.
echo [3/3] 正在拉起控制塔服务...
echo      物理数据库: DuckDB (ipc.db)
echo      求解内核: C++ 裸金属求解器 (main_mem3.exe)
echo.
echo =====================================================================
echo [运行中] 服务已启动，正在自动为您打开浏览器...
echo 访问地址: http://127.0.0.1:8501
echo =====================================================================
echo.

start http://127.0.0.1:8501
python server.py

pause
