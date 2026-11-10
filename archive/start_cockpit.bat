@echo off
title IPC Intelligent Planning Cockpit Server Startup

echo =====================================================================
echo          IPC Intelligent Planning Cockpit and LLM Copilot
echo =====================================================================
echo.
echo [1/3] Checking Python environment...
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Python not found in system PATH! Please install Python.
    pause
    exit /b 1
)

echo [2/3] Installing/Upgrading required Python API and Server packages...
echo      (Packages: fastapi, uvicorn, duckdb)
echo      Please wait, this will only take a few seconds...
python -m pip install --quiet --upgrade pip
python -m pip install --quiet fastapi uvicorn duckdb pandas plotly

echo.
echo [3/3] Launching IPC custom Visual Control Tower and LLM Copilot...
echo      Please wait. The server will start on http://127.0.0.1:8501.
echo      You can open http://127.0.0.1:8501 in your browser.
echo      Engine: C++ Bare-Metal Core (test_runner_validation.exe)
echo      Database: DuckDB Context (ipc.db)
echo.
echo =====================================================================
echo [RUNNING] Starting Custom Web App Server, please do not close this window...
echo =====================================================================

start http://127.0.0.1:8501
python server.py

pause
