#!/bin/bash
# scripts/build.sh

set -e

echo "========================================="
echo "CustomDB Build Script"
echo "========================================="

# Определяем, нужно ли использовать sudo (для контейнера или хоста)
if [ "$EUID" -eq 0 ]; then
    SUDO=""
else
    SUDO="sudo"
fi

# Определяем ОС
OS="$(uname -s)"
echo "Detected OS: $OS"

# Устанавливаем системные зависимости
install_packages() {
    if command -v apt &> /dev/null; then
        echo "Using apt package manager"
        $SUDO apt update
        $SUDO apt install -y build-essential cmake curl unzip tar zip pkg-config libgtest-dev nlohmann-json3-dev
    elif command -v brew &> /dev/null; then
        echo "Using brew package manager"
        brew install cmake
    elif command -v pacman &> /dev/null; then
        echo "Using pacman package manager"
        $SUDO pacman -S --noconfirm base-devel cmake zip unzip
    fi
}

echo ""
echo "Step 1: Installing system dependencies..."
install_packages

# Проверяем CMake
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake not found!"
    exit 1
fi
echo "CMake version: $(cmake --version | head -n1)"

# Создаём папку для сборки
echo ""
echo "Step 2: Configuring CMake..."
mkdir -p build
cd build

# Запускаем CMake (без vcpkg, используем системные библиотеки)
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17

# Собираем проект
echo ""
echo "Step 3: Building project..."
cmake --build . --config Release -j$(nproc)

echo ""
echo "========================================="
echo "Build completed successfully!"
echo "========================================="