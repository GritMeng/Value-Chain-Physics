@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"

echo ========================================================
echo   IPC Core Engine Benchmark - Build and Test (Windows)
echo ========================================================

where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] MSVC cl.exe not found. Please run in a Developer Command Prompt.
    exit /b 1
)

if not exist build mkdir build
if not exist bin mkdir bin

echo [1/5] Compiling core engine modules...
cl /std:c++17 /EHsc /O2 /I include /c src\data_loader.cpp src\atp_ctp_engine.cpp src\itp_iop_alignment.cpp src\substitution_engine.cpp /Fobuild\ /utf-8 >build\compile.log 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed. See build\compile.log
    type build\compile.log
    exit /b 1
)

echo [2/5] Linking benchmark executables...
set OBJS=build\data_loader.obj build\atp_ctp_engine.obj build\itp_iop_alignment.obj build\substitution_engine.obj
for %%T in (test_delivery_precision test_itp_iop_alignment test_substitution_rules stress_benchmark_2m) do (
    cl /std:c++17 /EHsc /O2 /I include benchmarks\%%T.cpp %OBJS% /Fe:bin\%%T.exe /utf-8 >>build\compile.log 2>&1
)

echo [3/5] Running C++ engine benchmarks...
echo.
bin\test_delivery_precision.exe
echo.
bin\test_itp_iop_alignment.exe
echo.
bin\test_substitution_rules.exe
echo.

echo [4/5] Running stress benchmark (requires data\*.csv)...
if exist data (
    bin\stress_benchmark_2m.exe data
) else (
    echo   [SKIP] data/ not found. Run: python data\generate_data.py
)
echo.

echo [5/5] Running Python cross validator...
python verifier\cross_validator.py

echo ========================================================
echo   All benchmarks and cross validations completed.
echo ========================================================
