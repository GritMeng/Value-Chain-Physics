@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

cd /d "%~dp0"

echo ================================================================
echo   IPC Core Open Benchmarks v2.2 - Build and Verify Suite (7 Suites)
echo ================================================================

set COMPILER_KIND=

where cl >nul 2>&1
if %errorlevel% equ 0 set COMPILER_KIND=MSVC

if "%COMPILER_KIND%"=="" (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\14.0\VC\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\14.0\VC\vcvarsall.bat" amd64 >nul 2>&1
    )
    where cl >nul 2>&1
    if !errorlevel! equ 0 set COMPILER_KIND=MSVC
)

if "%COMPILER_KIND%"=="" (
    where g++ >nul 2>&1
    if !errorlevel! equ 0 set COMPILER_KIND=GPP
)

if "%COMPILER_KIND%"=="" (
    where clang++ >nul 2>&1
    if !errorlevel! equ 0 set COMPILER_KIND=CLANGPP
)

if "%COMPILER_KIND%"=="" (
    echo [ERROR] No C++17 compiler found ^(MSVC cl.exe / MinGW g++ / clang++^).
    exit /b 1
)

if not exist build mkdir build
if not exist bin mkdir bin

echo [1/4] Compiling clean-room C++17 modules via %COMPILER_KIND%...
if "%COMPILER_KIND%"=="MSVC" (
    cl /std:c++17 /EHsc /O2 /W1 /I include /c src\substitution_engine.cpp src\atp_ctp_engine.cpp src\itp_iop_alignment.cpp src\sandbox_operators.cpp /Fobuild\ /utf-8 >build\compile.log 2>&1
    if !errorlevel! neq 0 (
        echo [ERROR] MSVC compilation failed. Check build\compile.log
        type build\compile.log
        exit /b 1
    )
    set OBJS=build\substitution_engine.obj build\atp_ctp_engine.obj build\itp_iop_alignment.obj build\sandbox_operators.obj
    echo [2/4] Linking 7 benchmark executables...
    for %%T in (test_delivery_precision test_substitution_rules test_codp_positioning test_push_pull_router test_itp_iop_alignment test_two_stage_iop_network stress_benchmark_2m) do (
        cl /std:c++17 /EHsc /O2 /I include benchmarks\%%T.cpp !OBJS! psapi.lib /Fe:bin\%%T.exe /utf-8 /Fo:build\ >>build\compile.log 2>&1
        if !errorlevel! neq 0 (
            echo [ERROR] Linking %%T failed.
            type build\compile.log
            exit /b 1
        )
    )
) else (
    set CXX_BIN=g++
    if "%COMPILER_KIND%"=="CLANGPP" set CXX_BIN=clang++
    >build\compile.log echo Using !CXX_BIN!
    for %%S in (substitution_engine atp_ctp_engine itp_iop_alignment sandbox_operators) do (
        !CXX_BIN! -std=c++17 -O3 -Wall -I include -c src\%%S.cpp -o build\%%S.o >>build\compile.log 2>&1
        if !errorlevel! neq 0 (
            echo [ERROR] Compiling %%S.cpp failed.
            type build\compile.log
            exit /b 1
        )
    )
    set OBJS=build\substitution_engine.o build\atp_ctp_engine.o build\itp_iop_alignment.o build\sandbox_operators.o
    echo [2/4] Linking 7 benchmark executables...
    for %%T in (test_delivery_precision test_substitution_rules test_codp_positioning test_push_pull_router test_itp_iop_alignment test_two_stage_iop_network stress_benchmark_2m) do (
        !CXX_BIN! -std=c++17 -O3 -Wall -I include benchmarks\%%T.cpp !OBJS! -lpsapi -o bin\%%T.exe >>build\compile.log 2>&1
        if !errorlevel! neq 0 (
            echo [ERROR] Linking %%T failed.
            type build\compile.log
            exit /b 1
        )
    )
)

echo [3/4] Running all 7 C++17 benchmarks...
for %%T in (test_delivery_precision test_substitution_rules test_codp_positioning test_push_pull_router test_itp_iop_alignment test_two_stage_iop_network stress_benchmark_2m) do (
    echo.
    bin\%%T.exe
    if !errorlevel! neq 0 (
        echo [FAILED] %%T exited with error code !errorlevel!.
        exit /b 1
    )
)

echo.
echo [4/4] Running Python Independent Mathematical Cross-Validator (11 Deterministic Checks)...
python verifier\cross_validator.py
if !errorlevel! neq 0 (
    echo [FAILED] Python Cross-Validator failed.
    exit /b 1
)

echo ================================================================
echo   [ALL PASSED] 7 C++17 Suites + 11/11 Python Cross-Checks Verified.
echo ================================================================
