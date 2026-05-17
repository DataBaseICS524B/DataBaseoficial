@echo off
setlocal enabledelayedexpansion

echo  CustomDB Build Script for Windows 

if "%VCPKG_ROOT%"=="" set VCPKG_ROOT=C:\vcpkg

if exist build rmdir /s /q build

:: 1. Сборка C++ сервера
echo [1/4] Building C++ server...
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b !errorlevel!
cmake --build . --config Release
if errorlevel 1 exit /b !errorlevel!
cd ..

:: 2. Публикация GUI (автономный .exe для Windows)
echo [2/4] Publishing GUI for Windows...
cd src\gui
dotnet publish CustomDB.UI.csproj -c Release --self-contained -r win-x64 -o ..\..\publish\win-x64
if errorlevel 1 exit /b !errorlevel!
cd ..\..

:: 3. Копирование сервера в папку с GUI
echo [3/4] Copying server binary to GUI folder...
copy build\Release\customdb_server.exe publish\win-x64\
if errorlevel 1 exit /b !errorlevel!

:: 4. Сборка Docker-образа
echo [4/4] Building Docker image...
docker build -t databaseoficial-customdb-server .
if errorlevel 1 exit /b !errorlevel!

echo Build completed successfully!
echo GUI location: publish\win-x64\CustomDB.UI.exe
echo Server location: build\Release\customdb_server.exe
echo Docker image: databaseoficial-customdb-server
endlocal