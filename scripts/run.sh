#!/bin/bash
# scripts/run.sh

# Параметры по умолчанию
HOST="localhost"
PORT="5432"

# Парсим аргументы
while [[ $# -gt 0 ]]; do
    case $1 in
        --host)
            HOST="$2"
            shift 2
            ;;
        --port)
            PORT="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: ./run.sh --host <host> --port <port>"
            exit 1
            ;;
    esac
done

echo "Starting CustomDB Server on $HOST:$PORT"

# Проверяем существует ли сервер
if [ ! -f "build/customdb_server" ] && [ ! -f "build/customdb_server.exe" ]; then
    echo "Server binary not found. Running build.sh first..."
    ./scripts/build.sh
fi

# Создаём папку для данных если её нет
mkdir -p data

# Запускаем сервер
if [ -f "build/customdb_server" ]; then
    ./build/customdb_server --host "$HOST" --port "$PORT"
elif [ -f "build/customdb_server.exe" ]; then
    ./build/customdb_server.exe --host "$HOST" --port "$PORT"
else
    echo "Error: Could not find server binary"
    exit 1
fi