@echo off
setlocal enabledelayedexpansion

cd /d "%~dp0"

:: Auto-initialize Visual Studio developer environment if not already loaded
where cl >nul 2>&1
if %errorlevel% neq 0 (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    )
)

echo.
echo =====================================================================
echo      IPC ITP/IOP Standalone Stress Test - Build and Run Script
echo =====================================================================
echo.

:: Check compiler
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] MSVC compiler cl.exe not found.
    echo         Please run this script from VS Developer Command Prompt.
    exit /b 1
)

:: Prepare directories
if not exist build mkdir build
if not exist bin   mkdir bin

:: Compilation flags
set CFLAGS=/std:c++20 /O2 /utf-8 /W1 /I include /I IPC\include

:: Compile itp_iop_stress_test.cpp
echo [1/2] Compiling itp_iop_stress_test.cpp (standalone) ...
cl %CFLAGS% /c tests\itp_iop_stress_test.cpp /Fobuild\itp_iop_stress.obj >build\itp_iop_stress.log 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] itp_iop_stress_test.cpp compilation failed. Check build\itp_iop_stress.log
    type build\itp_iop_stress.log
    exit /b 1
)
echo        [OK] itp_iop_stress.obj

:: Link
echo [2/2] Linking bin\itp_iop_stress.exe ...
link /OUT:bin\itp_iop_stress.exe build\itp_iop_stress.obj duckdb.lib /LIBPATH:IPC\lib /SUBSYSTEM:CONSOLE >build\itp_iop_link.log 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Linking failed. Check build\itp_iop_link.log
    type build\itp_iop_link.log
    exit /b 1
)
echo        [OK] bin\itp_iop_stress.exe

:: Run
echo.
echo =====================================================================
echo      Running ITP/IOP Stress Test...
echo =====================================================================
echo.

set START_TIME=%TIME%
bin\itp_iop_stress.exe
set EXIT_CODE=%errorlevel%
set END_TIME=%TIME%

echo.
echo Start Time: %START_TIME%
echo End Time:   %END_TIME%

if %EXIT_CODE% equ 0 (
    echo =====================================================================
    echo      [ALL PASS] ITP/IOP stress test completed successfully!
    echo =====================================================================
) else (
    echo =====================================================================
    echo      [FAILURE] Some test cases failed. See output above.
    echo =====================================================================
)

exit /b %EXIT_CODE%
