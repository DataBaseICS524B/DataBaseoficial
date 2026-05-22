set -e

echo " Скрипт сборки CustomDB для Linux/macOS "

OS=$(uname -s)

echo "[0/3] Установка зависимостей (nlohmann-json)..."
if [ "$OS" = "Linux" ]; then
    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y nlohmann-json3-dev
    elif command -v yum &> /dev/null; then
        sudo yum install -y nlohmann-json-devel
    else
        echo "Пожалуйста, установите nlohmann-json-dev вручную"
    fi
elif [ "$OS" = "Darwin" ]; then
    if command -v brew &> /dev/null; then
        brew install nlohmann-json
    else
        echo "Пожалуйста, установите nlohmann-json через Homebrew"
    fi
fi

echo "[1/3] Сборка C++ сервера..."
cd "$(dirname "$0")/.."
rm -rf build 
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..

echo "[2/3] Копирование бинарного файла сервера..."
if [ "$OS" = "Linux" ]; then
    mkdir -p publish/linux-x64
    cp build/customdb_server publish/linux-x64/
elif [ "$OS" = "Darwin" ]; then
    mkdir -p publish/osx-x64
    cp build/customdb_server publish/osx-x64/
fi

echo "[3/3] Сборка Docker-образа..."
docker build -t databaseoficial-customdb-server .

echo "Сборка успешно завершена!"
if [ "$OS" = "Linux" ]; then
    echo "Сервер собран: build/customdb_server"
    echo "Скопирован в: publish/linux-x64/customdb_server"
elif [ "$OS" = "Darwin" ]; then
    echo "Сервер собран: build/customdb_server"
    echo "Скопирован в: publish/osx-x64/customdb_server"
fi
echo "Docker образ: databaseoficial-customdb-server"