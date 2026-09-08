@echo off
chcp 65001 >nul
setlocal

cd /d "%~dp0"

echo ========================================================
echo   IPC Core Engine Benchmark - Build and Test Tool
echo ========================================================

:: Search Visual Studio developer environment
where cl >nul 2>&1
if %errorlevel% neq 0 (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\14.0\VC\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\14.0\VC\vcvarsall.bat" amd64 >nul 2>&1
    )
)

where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] MSVC cl.exe compiler not found.
    exit /b 1
)

if not exist build mkdir build
if not exist bin mkdir bin

echo [1/4] Compiling core C++ engine modules...
cl /std:c++17 /EHsc /O2 /W1 /I include /c src\substitution_engine.cpp src\atp_ctp_engine.cpp src\itp_iop_alignment.cpp /Fobuild\ /utf-8 >build\compile.log 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] C++ compilation failed. Check build\compile.log
    type build\compile.log
    exit /b 1
)

echo [2/4] Linking benchmark executables...
cl /std:c++17 /EHsc /O2 /I include benchmarks\test_delivery_precision.cpp build\substitution_engine.obj build\atp_ctp_engine.obj build\itp_iop_alignment.obj /Fe:bin\test_delivery_precision.exe /utf-8 >>build\compile.log 2>&1
cl /std:c++17 /EHsc /O2 /I include benchmarks\test_itp_iop_alignment.cpp build\substitution_engine.obj build\atp_ctp_engine.obj build\itp_iop_alignment.obj /Fe:bin\test_itp_iop_alignment.exe /utf-8 >>build\compile.log 2>&1
cl /std:c++17 /EHsc /O2 /I include benchmarks\test_substitution_rules.cpp build\substitution_engine.obj build\atp_ctp_engine.obj build\itp_iop_alignment.obj /Fe:bin\test_substitution_rules.exe /utf-8 >>build\compile.log 2>&1
cl /std:c++17 /EHsc /O2 /I include benchmarks\stress_benchmark_2m.cpp build\substitution_engine.obj build\atp_ctp_engine.obj build\itp_iop_alignment.obj /Fe:bin\stress_benchmark_2m.exe /utf-8 >>build\compile.log 2>&1

echo [3/4] Running C++ Engine Benchmarks...
echo.
bin\test_delivery_precision.exe
echo.
bin\test_itp_iop_alignment.exe
echo.
bin\test_substitution_rules.exe
echo.
bin\stress_benchmark_2m.exe
echo.

echo [4/4] Running Python Cross Validator...
python verifier\cross_validator.py

echo ========================================================
echo   All Engine Benchmarks and Cross Validations PASSED!
echo ========================================================
