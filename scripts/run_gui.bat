@echo off
echo ========================================
echo Running CustomDB GUI Client
echo ========================================
echo.

cd /d "%~dp0\.."

:: Опционально: раскомментируйте следующую строку, чтобы пересобирать перед каждым запуском
:: call scripts\build_gui.bat
:: if %errorlevel% neq 0 exit /b %errorlevel%

:: Путь к собранному exe (измените под ваш .NET版本 и конфигурацию)
set GUI_EXE="src\gui\bin\Release\net8.0-windows\CustomDB.UI.exe"

:: Альтернативный поиск, если путь отличается
if not exist %GUI_EXE% (
    echo [WARNING] Default executable not found at %GUI_EXE%
    echo Searching for CustomDB.UI.exe...
    for /f "delims=" %%i in ('dir /s /b src\gui\bin\*.exe 2^>nul ^| findstr /i "CustomDB.UI.exe"') do set GUI_EXE="%%i"
)

if not exist %GUI_EXE% (
    echo [ERROR] Could not find CustomDB.UI.exe. Please build the project first.
    echo Run scripts\build_gui.bat
    pause
    exit /b 1
)

echo Starting GUI client...
echo Make sure the C++ server is already running (on localhost:8080 by default)
echo.

start "" %GUI_EXE%

echo GUI client launched.
