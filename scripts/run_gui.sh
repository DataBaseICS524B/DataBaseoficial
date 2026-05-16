#!/bin/bash

# Получаем директорию скрипта и переходим в корень проекта
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."
cd "$PROJECT_ROOT"

echo "========================================"
echo "Building and Running CustomDB GUI"
echo "========================================"

# Определяем путь к проекту
if [ -f "CustomDB.GUI.sln" ]; then
    SOLUTION_FILE="CustomDB.GUI.sln"
elif [ -f "src/gui/CustomDB.UI.csproj" ]; then
    PROJECT_FILE="src/gui/CustomDB.UI.csproj"
else
    echo "[ERROR] Cannot find GUI project or solution"
    exit 1
fi

# Проверяем dotnet
if ! command -v dotnet &> /dev/null; then
    echo "[ERROR] .NET SDK is not installed"
    echo "Install from https://dotnet.microsoft.com/download"
    exit 1
fi

echo "[1/3] Restoring packages..."
if [ -n "$SOLUTION_FILE" ]; then
    dotnet restore "$SOLUTION_FILE"
else
    dotnet restore "$PROJECT_FILE"
fi
if [ $? -ne 0 ]; then
    echo "[ERROR] Restore failed"
    exit 1
fi

echo "[2/3] Building project..."
if [ -n "$SOLUTION_FILE" ]; then
    dotnet build "$SOLUTION_FILE" --configuration Release --no-restore
else
    dotnet build "$PROJECT_FILE" --configuration Release --no-restore
fi
if [ $? -ne 0 ]; then
    echo "[ERROR] Build failed"
    exit 1
fi

echo "[3/3] Launching GUI..."
# Ищем исполняемый файл
GUI_EXE=$(find src/gui/bin/Release -name "CustomDB.UI" -type f 2>/dev/null | head -n 1)

if [ -z "$GUI_EXE" ]; then
    echo "[ERROR] Could not find GUI executable. Try running 'dotnet run' manually in src/gui/"
    echo "Example: cd src/gui && dotnet run"
    exit 1
fi

echo "Starting GUI client from: $GUI_EXE"
echo "Make sure the C++ server is running (localhost:8080)"
"$GUI_EXE" &

echo "GUI launched (PID: $!)"
