@echo off
set "PATH=C:\mingw64\bin;%PATH%"

if not exist run_demo.exe (
    echo Compiling run_demo.exe...
    g++ -std=c++17 -I"include" "src/Canonicalizer.cpp" "src/Fingerprint.cpp" "src/CacheManager.cpp" "src/ASTParser.cpp" "src/run_demo.cpp" -o "run_demo.exe"
)

if %ERRORLEVEL% EQU 0 (
    run_demo.exe
) else (
    echo [ERROR] Could not build or run run_demo.exe
)
