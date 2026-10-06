@echo off
setlocal

cd /d "%~dp0"

:: Auto-initialize Visual Studio developer environment if not already loaded
where cl >nul 2>&1
if %errorlevel% neq 0 (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\14.0\VC\vcvarsall.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\14.0\VC\vcvarsall.bat" amd64 >nul 2>&1
    ) else (
        :: Try finding any visual studio build tools vcvars64
        for /d %%i in ("C:\Program Files\Microsoft Visual Studio\*") do (
            if exist "%%i\Community\VC\Auxiliary\Build\vcvars64.bat" (
                call "%%i\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
                goto :found_vc
            )
            if exist "%%i\Professional\VC\Auxiliary\Build\vcvars64.bat" (
                call "%%i\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
                goto :found_vc
            )
            if exist "%%i\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
                call "%%i\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
                goto :found_vc
            )
        )
    )
)
:found_vc

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
echo [1/2] Compiling all C++ core engine modules ...
cl /std:c++20 /EHsc /O2 /W1 /I include /I IPC\include /c ^
    src\globals.cpp ^
    src\vocab.cpp ^
    src\math_utils.cpp ^
    src\substitution.cpp ^
    src\lsc_tree.cpp ^
    src\mrp_engine.cpp ^
    src\database.cpp ^
    src\dbd_engine.cpp ^
    src\coproduct.cpp ^
    src\tests.cpp ^
    src\csv_export.cpp ^
    src\scheduling.cpp ^
    src\hierarchy.cpp ^
    src\engine_main.cpp ^
    tests\test_stress.cpp ^
    /Fobuild\ /utf-8 >build\core_compile.log 2>&1

if %errorlevel% neq 0 (
    echo [ERROR] C++ compilation failed. Check build\core_compile.log
    type build\core_compile.log
    exit /b 1
)
echo        [OK] Core engine modules (.obj)

:: Link
echo [2/2] Linking main_mem3.exe and test_runner_validation.exe ...
link /OUT:main_mem3.exe ^
    build\globals.obj ^
    build\vocab.obj ^
    build\math_utils.obj ^
    build\substitution.obj ^
    build\lsc_tree.obj ^
    build\mrp_engine.obj ^
    build\database.obj ^
    build\dbd_engine.obj ^
    build\coproduct.obj ^
    build\tests.obj ^
    build\csv_export.obj ^
    build\scheduling.obj ^
    build\hierarchy.obj ^
    build\engine_main.obj ^
    build\test_stress.obj ^
    duckdb.lib /LIBPATH:IPC\lib /DEBUG:NONE >build\core_link.log 2>&1

if %errorlevel% neq 0 (
    echo [ERROR] Linking failed. Check build\core_link.log
    type build\core_link.log
    exit /b 1
)

:: Copy to test_runner_validation.exe as well
copy /y main_mem3.exe test_runner_validation.exe >nul
echo        [OK] Compiled main_mem3.exe & test_runner_validation.exe successfully!
exit /b 0
