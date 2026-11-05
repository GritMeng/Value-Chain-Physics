@echo off
:: =====================================================================
:: IPC 智能计划控制塔一键打包打包分发脚本 (Distributor Packager)
:: =====================================================================
chcp 65001 >nul
title IPC Distribution Packager
color 0B
setlocal enabledelayedexpansion

echo =====================================================================
echo          IPC 智能计划控制塔 - 企业级分发打包工具
echo =====================================================================
echo.

set "DIST_DIR=dist\ipc_platform"

echo [*] 正在清空并初始化分发目录 %DIST_DIR%...
if exist "dist" rmdir /s /q "dist"
mkdir "%DIST_DIR%"
mkdir "%DIST_DIR%\templates"

echo [*] 正在复制核心 C++ 裸金属求解器...
if not exist "test_runner_validation.exe" (
    echo [警告] 未检测到已编译的 test_runner_validation.exe！
)
copy /y "test_runner_validation.exe" "%DIST_DIR%\" >nul
if not exist "main_mem3.exe" (
    echo [警告] 未检测到已编译的 main_mem3.exe！
)
copy /y "main_mem3.exe" "%DIST_DIR%\" >nul

echo [*] 正在复制 Python 控制塔服务...
copy /y "server.py" "%DIST_DIR%\" >nul

echo [*] 正在复制 DuckDB 计划数据库...
copy /y "ipc.db" "%DIST_DIR%\" >nul

echo [*] 正在复制前端控制塔大盘模板...
xcopy /s /e /y "templates" "%DIST_DIR%\templates" >nul

echo [*] 正在复制现代 React 19 编译网页...
mkdir "%DIST_DIR%\frontend\dist" >nul 2>&1
xcopy /s /e /y "frontend\dist" "%DIST_DIR%\frontend\dist" >nul

echo [*] 正在复制 API 接口与集成文件...
copy /y "include\ipc_types.h" "%DIST_DIR%\" >nul

echo [*] 正在注入安装向导脚本 (install.bat)...
copy /y "install.bat" "%DIST_DIR%\" >nul

:: 创建一个 README 文件告诉客户怎么跑
echo # IPC 智能计划控制塔企业级部署包 > "%DIST_DIR%\README.md"
echo. >> "%DIST_DIR%\README.md"
echo 本安装包包含了 IPC 智能计划控制塔的全部核心组件，包括 C++ 裸金属有限产能求解器内核与 Python 可视化控制塔大盘。 >> "%DIST_DIR%\README.md"
echo. >> "%DIST_DIR%\README.md"
echo ## 🚀 极速安装步骤 >> "%DIST_DIR%\README.md"
echo 1. 右键以 **管理员身份运行** `install.bat` 脚本。 >> "%DIST_DIR%\README.md"
echo 2. 根据提示输入安装路径（默认安装在 `C:\IPC_Platform`）。 >> "%DIST_DIR%\README.md"
echo 3. 安装程序会自动完成环境检测、第三方依赖库安装、以及桌面快捷方式创建。 >> "%DIST_DIR%\README.md"
echo 4. 双击桌面生成的 `IPC_Control_Tower.lnk` 快捷方式即可一键启动控制塔引擎。 >> "%DIST_DIR%\README.md"
echo. >> "%DIST_DIR%\README.md"
echo ## 🌐 开发者 API 集成说明 >> "%DIST_DIR%\README.md"
echo 本包内附带了 C++ 核心接口定义文件 `ipc_types.h`。如果您需要将本引擎集成至您自研的 ERP（如华为 MetaERP、美的美云智数），请参考头文件定义的 `reserve_atp_and_capacity_recursive` 接口，配合 DLL 动态链接库载入。 >> "%DIST_DIR%\README.md"
echo. >> "%DIST_DIR%\README.md"
echo ===================================================================== >> "%DIST_DIR%\README.md"
echo IPC 智能计划控制塔 - 全局知识产权保护产品 >> "%DIST_DIR%\README.md"

echo.
echo =====================================================================
echo [成功] IPC 智能分发安装包已成功打包完成！
echo =====================================================================
echo  打包路径: %CD%\%DIST_DIR%
echo  您可以直接将 %DIST_DIR% 文件夹压缩并发送给客户服务器进行安装部署。
echo =====================================================================
echo.
pause
