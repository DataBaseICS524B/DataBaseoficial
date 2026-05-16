#!/bin/bash
# Скрипт для копирования GUI на Windows и запуска

DEST="/mnt/c/CustomDB_GUI"
SRC="$HOME/DataBaseoficial/src/gui"

echo "📁 Копирование GUI из WSL в C:\CustomDB_GUI ..."

# Удаляем старую папку, чтобы не было конфликтов
rm -rf "$DEST" 2>/dev/null
mkdir -p "$DEST"

# Копируем исходники, исключая мусорные папки
rsync -av --exclude='bin' --exclude='obj' --exclude='.vs' "$SRC/" "$DEST/"

if [ $? -ne 0 ]; then
    echo "❌ Ошибка копирования. Возможно, rsync не установлен. Установите: sudo apt install rsync"
    exit 1
fi

echo "✅ Копирование завершено."

# Проверяем, есть ли .NET на Windows
if ! cmd.exe /c "where dotnet" > /dev/null 2>&1; then
    echo "⚠️  .NET SDK не найден на Windows."
    echo "Скачайте и установите .NET 8.0 SDK: https://dotnet.microsoft.com/en-us/download/dotnet/8.0"
    echo "После установки запустите вручную:"
    echo "  cd C:\CustomDB_GUI"
    echo "  dotnet run"
    exit 0
fi

echo "🚀 Запуск GUI на Windows..."
# Открываем новое окно cmd, переходим в папку и запускаем dotnet run
cmd.exe /c "start cmd.exe /k \"cd /d C:\\CustomDB_GUI && echo CustomDB GUI started... && dotnet run\""

echo "✅ Если окно не открылось, выполните вручную в Windows:"
echo "   cd C:\\CustomDB_GUI"
echo "   dotnet run"