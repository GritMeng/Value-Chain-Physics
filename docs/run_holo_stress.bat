@echo off
setlocal

cd /d "%~dp0"

:: Auto-initialize Visual Studio developer environment if not already loaded
where cl >nul 2>&1
if %errorlevel% neq 0 (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    )
)

echo.
echo ==================================================================
echo      IPC Holographic Stress Test - Build and Run Script
echo ==================================================================
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

:: Compile core modules
echo [1/3] Compiling core engine modules ...
cl /std:c++20 /EHsc /O2 /openmp /W1 /I include /I IPC\include /c src\globals.cpp src\vocab.cpp src\math_utils.cpp src\substitution.cpp src\lsc_tree.cpp src\mrp_engine.cpp src\database.cpp src\dbd_engine.cpp src\coproduct.cpp src\tests.cpp src\csv_export.cpp src\scheduling.cpp src\hierarchy.cpp /Fobuild\ /utf-8 >build\core_compile.log 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Core engine modules compilation failed. Check build\core_compile.log
    type build\core_compile.log
    exit /b 1
)
echo        [OK] Core engine modules (.obj)

:: Compile holographic_stress_test.cpp
echo [2/3] Compiling holographic_stress_test.cpp ...
cl /std:c++20 /EHsc /O2 /openmp /W1 /I include /I IPC\include /c tests\holographic_stress_test.cpp /Fobuild\holo_stress.obj /utf-8 >build\holo_stress_compile.log 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] holographic_stress_test.cpp compilation failed. Check build\holo_stress_compile.log
    type build\holo_stress_compile.log
    exit /b 1
)
echo        [OK] holo_stress.obj

:: Link
echo [3/3] Linking bin\holo_stress.exe ...
link /OUT:bin\holo_stress.exe build\globals.obj build\vocab.obj build\math_utils.obj build\substitution.obj build\lsc_tree.obj build\mrp_engine.obj build\database.obj build\dbd_engine.obj build\coproduct.obj build\tests.obj build\csv_export.obj build\scheduling.obj build\hierarchy.obj build\holo_stress.obj duckdb.lib /LIBPATH:IPC\lib /NODEFAULTLIB:MSVCRT /DEBUG:NONE >build\holo_stress_link.log 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Linking failed. Check build\holo_stress_link.log
    type build\holo_stress_link.log
    exit /b 1
)
echo        [OK] bin\holo_stress.exe

:: Run
echo.
echo ==================================================================
echo      Running Holographic Stress Test...
echo ==================================================================
echo.

bin\holo_stress.exe
set EXITCODE=%errorlevel%

echo.
if %EXITCODE% equ 0 (
    echo ==================================================================
    echo      [ALL PASS] Holographic stress test completed successfully!
    echo ==================================================================
) else (
    echo ==================================================================
    echo      [FAILURE] Some test cases failed. See output above.
    echo ==================================================================
)

exit /b %EXITCODE%
