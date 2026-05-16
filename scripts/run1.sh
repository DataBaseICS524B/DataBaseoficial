#!/bin/bash
# run1.sh - Запуск консольного C# клиента

cd "$(dirname "$0")"

# Проверяем, запущен ли сервер
if ! nc -z localhost 5432 2>/dev/null; then
    echo "⚠️  Сервер не запущен на localhost:5432"
    echo "Запусти сервер в другом терминале: cd build && ./customdb_server"
    exit 1
fi

# Запускаем консольный клиент
cd src/ConsoleClient
dotnet run