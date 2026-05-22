@echo off
setlocal enabledelayedexpansion

echo Running ALL tests: Unit + Integration + Functional

cd /d "%~dp0"

:: 0. Пересборка тестов (без полной пересборки проекта)
echo [INFO] Rebuilding tests...
if not exist "build\CMakeCache.txt" (
    echo [ERROR] Project not configured. Run build_full.bat first.
    exit /b 1
)
cd build
cmake --build . --config Release --target unit_tests integration_tests
if errorlevel 1 (
    echo [ERROR] Test build failed.
    exit /b 1
)
cd ..

:: 1. Unit tests
echo.
echo [1/3] Running unit tests (C++ Google Test)
build\tests\Release\unit_tests.exe
if errorlevel 1 (
    echo [ERROR] Unit tests failed.
    exit /b 1
)

:: 2. Integration tests
echo.
echo [2/3] Running integration tests (C++ Google Test)
build\tests\Release\integration_tests.exe
if errorlevel 1 (
    echo [ERROR] Integration tests failed.
    exit /b 1
)

:: 3. Functional tests (Python)
echo.
echo [3/3] Running functional tests (Python)
python functional_test.py
if errorlevel 1 (
    echo [ERROR] Functional tests failed.
    exit /b 1
)

echo.
echo ALL TESTS PASSED SUCCESSFULLY!
exit /b 0