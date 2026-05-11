@echo off
echo ========================================
echo Building CustomDB C# GUI Application
echo ========================================
echo.

:: Переходим в корневую директорию проекта (на уровень выше scripts)
cd /d "%~dp0\.."

:: Проверяем, существует ли файл решения или проект
if exist "CustomDB.GUI.sln" (
    set SOLUTION_FILE="CustomDB.GUI.sln"
) else if exist "src\gui\CustomDB.UI.csproj" (
    set PROJECT_FILE="src\gui\CustomDB.UI.csproj"
) else (
    echo [ERROR] Could not find solution or project file for GUI.
    echo Expected: CustomDB.GUI.sln or src/gui/CustomDB.UI.csproj
    pause
    exit /b 1
)

:: Проверяем наличие dotnet
where dotnet >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] .NET SDK is not installed or not in PATH.
    echo Please install .NET SDK from https://dotnet.microsoft.com/download
    pause
    exit /b 1
)

echo [1/3] Restoring NuGet packages...
if defined SOLUTION_FILE (
    dotnet restore %SOLUTION_FILE%
) else (
    dotnet restore %PROJECT_FILE%
)
if %errorlevel% neq 0 (
    echo [ERROR] Package restore failed.
    pause
    exit /b 1
)

echo.
echo [2/3] Building GUI project...
if defined SOLUTION_FILE (
    dotnet build %SOLUTION_FILE% --configuration Release --no-restore
) else (
    dotnet build %PROJECT_FILE% --configuration Release --no-restore
)
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo [3/3] Build completed successfully!
echo GUI executable can be found in: src\gui\bin\Release\net8.0-windows\ (or similar)
echo.
pause
