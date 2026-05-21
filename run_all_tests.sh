#!/bin/bash
set -e

echo "Running ALL tests: Unit + Integration + Functional"

# Переходим в корень проекта (папка, где лежит скрипт)
cd "$(dirname "$0")"

# Функция для проверки успешности предыдущей команды
check_error() {
    if [ $? -ne 0 ]; then
        echo "[ERROR] $1"
        exit 1
    fi
}

# Принудительная пересборка проекта (актуальные тесты)
echo "[INFO] Rebuilding project..."
if [ -f "scripts/build_full.sh" ]; then
    bash scripts/build_full.sh
    check_error "Build failed"
else
    echo "[ERROR] build_full.sh not found in scripts/"
    exit 1
fi

# 1. Unit tests (C++ Google Test)
echo ""
echo "[1/3] Running unit tests (C++ Google Test)"
if [ -f "build/tests/unit_tests" ]; then
    ./build/tests/unit_tests
elif [ -f "build/tests/Release/unit_tests" ]; then
    ./build/tests/Release/unit_tests
elif [ -f "build/tests/Release/unit_tests.exe" ]; then
    # Windows binary under WSL? попробуем через wine? но лучше не надо
    echo "[ERROR] unit_tests binary not found in expected location"
    exit 1
else
    echo "[ERROR] unit_tests executable not found"
    exit 1
fi
check_error "Unit tests failed"

# 2. Integration tests (C++ Google Test)
echo ""
echo "[2/3] Running integration tests (C++ Google Test)"
if [ -f "build/tests/integration_tests" ]; then
    ./build/tests/integration_tests
elif [ -f "build/tests/Release/integration_tests" ]; then
    ./build/tests/Release/integration_tests
elif [ -f "build/tests/Release/integration_tests.exe" ]; then
    echo "[ERROR] integration_tests binary not found"
    exit 1
else
    echo "[ERROR] integration_tests executable not found"
    exit 1
fi
check_error "Integration tests failed"

# 3. Functional tests (Python)
echo ""
echo "[3/3] Running functional tests (Python)"
# Проверяем наличие Python3
if command -v python3 &> /dev/null; then
    python3 functional_test.py
elif command -v python &> /dev/null; then
    python functional_test.py
else
    echo "[ERROR] Python not found"
    exit 1
fi
check_error "Functional tests failed"

echo ""
echo "ALL TESTS PASSED SUCCESSFULLY!"