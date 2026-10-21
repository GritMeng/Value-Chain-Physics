@echo off
rem Build both binaries
make all

rem Run data generation
if exist bin\generate_data.exe (
    echo Generating synthetic data...
    bin\generate_data.exe
) else (
    echo [WARNING] generate_data.exe not found.
)

rem Run IPC engine
if exist bin\ipc_engine.exe (
    echo Running IPC engine...
    bin\ipc_engine.exe
) else (
    echo [WARNING] ipc_engine.exe not found.
)

rem Optional: export results to CSV (example)
rem duckdb ipc.db "COPY results TO 'results.csv' (FORMAT CSV);"
