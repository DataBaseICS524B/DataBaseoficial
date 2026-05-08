#!/bin/bash
# scripts/build.sh

set -e

echo "========================================="
echo "CustomDB Build Script"
echo "========================================="

# Определяем ОС
OS="$(uname -s)"
echo "Detected OS: $OS"

# Устанавливаем системные зависимости
install_packages() {
    if command -v apt &> /dev/null; then
        echo "Using apt package manager"
        sudo apt update
        sudo apt install -y build-essential cmake curl unzip tar zip pkg-config
    elif command -v brew &> /dev/null; then
        echo "Using brew package manager"
        brew install cmake
    elif command -v pacman &> /dev/null; then
        echo "Using pacman package manager"
        sudo pacman -S --noconfirm base-devel cmake zip unzip
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

# Устанавливаем vcpkg если нужно
if [ ! -d "vcpkg" ]; then
    echo ""
    echo "Step 2: Installing vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git
    ./vcpkg/bootstrap-vcpkg.sh
else
    echo ""
    echo "Step 2: vcpkg already installed"
fi

# Устанавливаем зависимости через манифест (просто запускаем vcpkg install без аргументов)
echo ""
echo "Step 3: Installing dependencies via vcpkg (manifest mode)..."
./vcpkg/vcpkg install --triplet x64-linux

# Создаём папку для сборки
echo ""
echo "Step 4: Configuring CMake..."
mkdir -p build
cd build

# Запускаем CMake с vcpkg toolchain
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=17

# Собираем проект
echo ""
echo "Step 5: Building project..."
cmake --build . --config Release -j$(nproc)

echo ""
echo "========================================="
echo "Build completed successfully!"
echo "========================================="