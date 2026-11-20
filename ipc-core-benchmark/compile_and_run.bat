@echo off
chcp 65001 >nul
setlocal

cd /d "%~dp0"

echo ================================================================
echo   IPC Core Open Benchmarks v2.0 - Build and Verify Suite
echo ================================================================

where cl >nul 2>&1
if %errorlevel% neq 0 (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\14.0\VC\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\14.0\VC\vcvarsall.bat" amd64 >nul 2>&1
    )
)

where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] MSVC cl.exe compiler not found. On Linux/macOS use CMake + g++/clang++.
    exit /b 1
)

if not exist build mkdir build
if not exist bin mkdir bin

echo [1/4] Compiling clean-room C++17 modules...
cl /std:c++17 /EHsc /O2 /W1 /I include /c src\substitution_engine.cpp src\atp_ctp_engine.cpp src\itp_iop_alignment.cpp src\sandbox_operators.cpp /Fobuild\ /utf-8 >build\compile.log 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] C++ compilation failed. Check build\compile.log
    type build\compile.log
    exit /b 1
)

set OBJS=build\substitution_engine.obj build\atp_ctp_engine.obj build\itp_iop_alignment.obj build\sandbox_operators.obj

echo [2/4] Linking 6 benchmark executables...
for %%T in (test_delivery_precision test_substitution_rules test_codp_positioning test_push_pull_router test_itp_iop_alignment stress_benchmark_2m) do (
    cl /std:c++17 /EHsc /O2 /I include benchmarks\%%T.cpp %OBJS% /Fe:bin\%%T.exe /utf-8 >>build\compile.log 2>&1
    if errorlevel 1 (
        echo [ERROR] Linking %%T failed!
        type build\compile.log
        exit /b 1
    )
)

echo [3/4] Running all 6 C++17 benchmarks...
for %%T in (test_delivery_precision test_substitution_rules test_codp_positioning test_push_pull_router test_itp_iop_alignment stress_benchmark_2m) do (
    echo.
    bin\%%T.exe
    if errorlevel 1 (
        echo [FAILED] %%T exited with error!
        exit /b 1
    )
)

echo.
echo [4/4] Running Python Independent Mathematical Cross-Validator...
python verifier\cross_validator.py
if errorlevel 1 (
    echo [FAILED] Python Cross-Validator failed!
    exit /b 1
)

echo ================================================================
echo   [ALL PASSED] 6 C++17 Suites + Python Cross-Validator Verified!
echo ================================================================
