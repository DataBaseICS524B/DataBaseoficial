#!/bin/bash
set -e

echo "========================================="
echo "CustomDB - Installing Dependencies"
echo "========================================="

OS="$(uname -s)"
echo "Detected OS: $OS"

if command -v apt &> /dev/null; then
    echo "Using apt (Ubuntu/Debian)..."
    sudo apt update
    sudo apt install -y build-essential cmake curl unzip tar zip pkg-config libssl-dev

elif command -v brew &> /dev/null; then
    echo "Using brew (macOS)..."
    brew install cmake pkg-config openssl

elif command -v pacman &> /dev/null; then
    echo "Using pacman (Arch Linux)..."
    sudo pacman -S --noconfirm base-devel cmake zip unzip

else
    echo "Unsupported OS. Please install dependencies manually."
    exit 1
fi

# Установка vcpkg
if [ ! -d "vcpkg" ]; then
    echo "Installing vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git
    ./vcpkg/bootstrap-vcpkg.sh
fi

echo "Installing nlohmann-json via vcpkg..."
./vcpkg/vcpkg install nlohmann-json

echo "========================================="
echo "Dependencies installed successfully!"
echo "========================================="