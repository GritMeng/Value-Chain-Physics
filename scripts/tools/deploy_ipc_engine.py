import os
import shutil

dist_dir = os.path.join("dist", "ipc_platform")

print("[*] 正在清空并初始化分发目录...")
if os.path.exists("dist"):
    shutil.rmtree("dist")
os.makedirs(dist_dir)

print("[*] 正在复制核心 C++ 裸金属求解器...")
shutil.copy("test_runner_validation.exe", dist_dir)
if os.path.exists("main_mem3.exe"):
    shutil.copy("main_mem3.exe", dist_dir)

print("[*] 正在复制 Python 控制塔服务...")
shutil.copy("server.py", dist_dir)

print("[*] 正在复制 DuckDB 计划数据库...")
shutil.copy("ipc.db", dist_dir)

print("[*] 正在复制前端控制塔大盘模板...")
shutil.copytree("templates", os.path.join(dist_dir, "templates"), dirs_exist_ok=True)

print("[*] 正在复制现代 React 19 编译网页...")
shutil.copytree(os.path.join("frontend", "dist"), os.path.join(dist_dir, "frontend", "dist"), dirs_exist_ok=True)

print("[*] 正在复制 API 接口与集成文件...")
shutil.copy(os.path.join("include", "ipc_types.h"), dist_dir)

print("[*] 正在注入安装向导脚本 (install.bat)...")
shutil.copy("install.bat", dist_dir)

print("[*] 正在创建 README.md...")
readme_content = """# IPC 智能计划控制塔企业级部署包

本安装包包含了 IPC 智能计划控制塔的全部核心组件，包括 C++ 裸金属有限产能求解器内核与 Python 可视化控制塔大盘。

## 🚀 极速安装步骤
1. 右键以 **管理员身份运行** `install.bat` 脚本。
2. 根据提示输入安装路径（默认安装在 `C:\\IPC_Platform`）。
3. 安装程序会自动完成环境检测、第三方依赖库安装、以及桌面快捷方式创建。
4. 双击桌面生成的 `IPC_Control_Tower.lnk` 快捷方式即可一键启动控制塔引擎。

## 🌐 开发者 API 集成说明
本包内附带了 C++ 核心接口定义文件 `ipc_types.h`。如果您需要将本引擎集成至您自研的 ERP（如华为 MetaERP、美的美云智数），请参考头文件定义的 `reserve_atp_and_capacity_recursive` 接口，配合 DLL 动态链接库载入。

=====================================================================
IPC 智能计划控制塔 - 全局知识产权保护产品
"""

with open(os.path.join(dist_dir, "README.md"), "w", encoding="utf-8") as f:
    f.write(readme_content)

print("=====================================================================")
print("[成功] IPC 智能分发安装包已成功打包完成！")
print(f" 打包路径: {os.path.abspath(dist_dir)}")
print("=====================================================================")
