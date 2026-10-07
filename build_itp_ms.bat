@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d h:\IPC
set CFLAGS=/std:c++20 /O2 /openmp /utf-8 /W1 /I include /I IPC\include
if not exist build mkdir build
if not exist bin   mkdir bin

echo [1/2] Compiling itp_iop_stress_test.cpp (standalone)...
cl %CFLAGS% /c tests\itp_iop_stress_test.cpp /Fobuild\itp_iop_stress.obj 2>build\itp_iop_stress.log
if %errorlevel% neq 0 (
    echo [ERROR] compile failed:
    type build\itp_iop_stress.log
    exit /b 1
)
echo        [OK] itp_iop_stress.obj

echo [2/2] Linking...
link /OUT:bin\itp_iop_stress.exe build\itp_iop_stress.obj IPC\lib\duckdb.lib /LIBPATH:. /SUBSYSTEM:CONSOLE 2>build\itp_iop_link.log
if %errorlevel% neq 0 (
    echo [ERROR] Link failed:
    type build\itp_iop_link.log
    exit /b 1
)
echo        [OK] bin\itp_iop_stress.exe

echo.
echo Running itp_iop_stress.exe...
bin\itp_iop_stress.exe
exit /b %errorlevel%
