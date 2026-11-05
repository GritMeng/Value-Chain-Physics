@echo off
:: =====================================================================
:: IPC 智能计划控制塔与求解器引擎一键部署安装向导 (Distributable Installer)
:: =====================================================================
title IPC Platform Enterprise Installer
color 0A
setlocal enabledelayedexpansion

echo =====================================================================
echo    IPC 智能计划控制塔 (Intelligent Planning & Control) 一键安装向导
echo =====================================================================
echo.
echo  本脚本将为您在当前服务器/工作站部署 IPC 裸金属计划求解器与控制塔系统。
echo  系统环境要求: Windows 10 / Windows Server 2016 或更高版本, Python 3.8+
echo.
echo =====================================================================
echo.

:: 1. 权限检查
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [警告] 未检测到管理员权限！创建 Windows 服务或写入 Program Files 需要管理员权限。
    echo        请以管理员身份重新运行此脚本，或者按任意键继续（将安装至用户本地目录）...
    pause >nul
)

:: 2. 定义安装路径
set "DEFAULT_INSTALL_DIR=C:\IPC_Platform"
echo [*] 请输入安装目标目录 (默认: %DEFAULT_INSTALL_DIR%):
set /p "INSTALL_DIR="
if "%INSTALL_DIR%"=="" (
    set "INSTALL_DIR=%DEFAULT_INSTALL_DIR%"
)

echo [*] 正在创建安装目录: %INSTALL_DIR%...
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

:: 3. 复制核心程序
echo [*] 正在复制核心求解器与服务组件...
if exist "test_runner_validation.exe" (
    copy /y "test_runner_validation.exe" "%INSTALL_DIR%\" >nul
)
if exist "main_mem3.exe" (
    copy /y "main_mem3.exe" "%INSTALL_DIR%\" >nul
)
if exist "server.py" (
    copy /y "server.py" "%INSTALL_DIR%\" >nul
)
if exist "ipc_types.h" (
    copy /y "ipc_types.h" "%INSTALL_DIR%\" >nul
)
if exist "start_cockpit.bat" (
    copy /y "start_cockpit.bat" "%INSTALL_DIR%\" >nul
)
if exist "ipc.db" (
    copy /y "ipc.db" "%INSTALL_DIR%\" >nul
)

:: 创建前端模板目录
if not exist "%INSTALL_DIR%\templates" mkdir "%INSTALL_DIR%\templates"
if exist "templates" (
    xcopy /s /e /y "templates" "%INSTALL_DIR%\templates" >nul
)

:: 复制现代 React 19 编译网页
if not exist "%INSTALL_DIR%\frontend\dist" mkdir "%INSTALL_DIR%\frontend\dist"
if exist "frontend\dist" (
    xcopy /s /e /y "frontend\dist" "%INSTALL_DIR%\frontend\dist" >nul
)

:: 4. 环境配置与 Python 依赖安装
echo [*] 正在验证 Python 环境...
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 系统未检测到 Python 运行环境！请先安装 Python 并添加至 PATH。
    pause
    exit /b 1
)

echo [*] 正在为本地 Python 安装必要的运行依赖包...
echo     (依赖包含: fastapi, uvicorn, duckdb, pandas, plotly)
python -m pip install --quiet --upgrade pip
python -m pip install --quiet fastapi uvicorn duckdb pandas plotly

:: 5. 写入启动引导脚本
echo [*] 正在配置系统启动器...
(
echo @echo off
echo cd /d "%INSTALL_DIR%"
echo title IPC Control Tower Server
echo echo =====================================================================
echo echo          IPC Intelligent Planning Cockpit Server Running
echo echo =====================================================================
echo echo  Server is running on: http://127.0.0.1:8501
echo echo  Engine: C++ DOD Solver Core (test_runner_validation.exe)
echo echo  Database: DuckDB Context (ipc.db)
echo echo =====================================================================
echo python server.py
) > "%INSTALL_DIR%\run_server.bat"

:: 6. 创建桌面快捷方式
echo [*] 正在创建桌面快捷方式...
set "TEMP_PS=%TEMP%\create_shortcut.ps1"
(
echo $WshShell = New-Object -ComObject WScript.Shell
echo $Shortcut = $WshShell.CreateShortcut([System.IO.Path]::Combine([Environment]::GetFolderPath('Desktop'), 'IPC_Control_Tower.lnk'))
echo $Shortcut.TargetPath = "%INSTALL_DIR%\run_server.bat"
echo $Shortcut.WorkingDirectory = "%INSTALL_DIR%"
echo $Shortcut.IconLocation = "shell32.dll,23"
echo $Shortcut.Description = "IPC Intelligent Planning Control Tower"
echo $Shortcut.Save()
) > "%TEMP_PS%"
powershell -ExecutionPolicy Bypass -File "%TEMP_PS%" >nul 2>&1
del "%TEMP_PS%"

echo.
echo =====================================================================
echo [成功] IPC 智能计划控制塔一键安装部署完成！
echo =====================================================================
echo.
echo  1. 安装目录: %INSTALL_DIR%
echo  2. 桌面已生成快捷方式: IPC_Control_Tower.lnk
echo  3. 双击桌面快捷方式即可拉起控制塔大盘与 C++ 裸金属求解器。
echo  4. 启动后，请在浏览器中访问: http://127.0.0.1:8501
echo.
echo =====================================================================
echo 按任意键退出安装向导...
pause >nul
