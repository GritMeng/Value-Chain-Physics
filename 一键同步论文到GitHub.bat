@echo off
chcp 65001 > null
echo ==========================================
echo    全息抗熵 / 价值链物理学 一键同步工具
echo ==========================================
echo.
echo 正在检测本地修改...
git status
echo.
set /p msg="请输入本次修改的简短说明（直接回车将使用默认说明）："
if "%msg%"=="" set msg="自动同步书稿与论文"
echo.
echo 1. 正在将修改加入缓存区...
git add .
echo.
echo 2. 正在提交修改...
git commit -m %msg%
echo.
echo 3. 正在同步到 GitHub 云端（已开启防 SSL 报错机制）...
git -c http.sslVerify=false push
echo.
echo ==========================================
echo 同步完成！按任意键退出。
echo ==========================================
pause
