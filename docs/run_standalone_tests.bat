@echo off
setlocal

cd /d "%~dp0"

:: Auto-initialize Visual Studio developer environment
where cl >nul 2>&1
if %errorlevel% neq 0 (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    ) else (
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

where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] MSVC compiler cl.exe not found.
    exit /b 1
)

if not exist bin mkdir bin

cl /std:c++20 /EHsc /O2 /utf-8 /I include tests\test_runner_validation.cpp /Fe:bin\test_runner_validation_standalone.exe /Fd:bin\ >bin\compile_standalone.log 2>&1

if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed. Log:
    type bin\compile_standalone.log
    exit /b 1
)
echo        [OK] bin\test_runner_validation_standalone.exe

echo [2/2] Running standalone tests...
bin\test_runner_validation_standalone.exe
exit /b %errorlevel%
