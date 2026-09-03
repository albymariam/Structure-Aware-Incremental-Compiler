@echo off
echo =======================================================
echo Compiling Structure-Aware C++ Compiler (Review 1 Demo)
echo =======================================================

set "PATH=C:\mingw64\bin;%PATH%"

g++ -std=c++17 -I"include" "src/Canonicalizer.cpp" "src/Fingerprint.cpp" "src/CacheManager.cpp" "src/ASTParser.cpp" "src/run_demo.cpp" -o "run_demo.exe"

if %ERRORLEVEL% EQU 0 (
    echo [BUILD SUCCESS] run_demo.exe generated successfully!
    echo Running Native C++ Review 1 Demonstration...
    echo -------------------------------------------------------
    run_demo.exe
) else (
    echo [BUILD FAILED] Please check compiler settings.
)
