@echo off
setlocal enabledelayedexpansion

set EXE_NAME=esp32mc_win.exe
set TOOLCHAIN_DIR=%~dp0..\..\toolchain
set W64DEVKIT_DIR=%TOOLCHAIN_DIR%\w64devkit
set GCC=%W64DEVKIT_DIR%\bin\g++.exe
set DOWNLOAD_URL=https://github.com/skeeto/w64devkit/releases/download/v2.8.0/w64devkit-x64-2.8.0.7z.exe

if exist "%GCC%" goto :compile

echo [SETUP] Downloading portable compiler...
if not exist "%TOOLCHAIN_DIR%" mkdir "%TOOLCHAIN_DIR%"
set ARCHIVE=%TOOLCHAIN_DIR%\w64devkit.7z.exe
set HTTP_PROXY=http://127.0.0.1:7890
set HTTPS_PROXY=http://127.0.0.1:7890
curl.exe -L --proxy "%HTTP_PROXY%" -o "%ARCHIVE%" "%DOWNLOAD_URL%"
if %ERRORLEVEL% NEQ 0 ( echo [ERROR] Download failed! & pause & exit /b 1 )
"%ARCHIVE%" -o"%TOOLCHAIN_DIR%" -y
del /q "%ARCHIVE%" >nul 2>&1
if not exist "%GCC%" ( echo [ERROR] Compiler not found! & pause & exit /b 1 )

:compile
set PATH=%W64DEVKIT_DIR%\bin;%PATH%
echo [BUILD] Compiling ESP32MC (Windows)...
pushd "%~dp0"

"%GCC%" -m64 -O2 -D_WIN32 -DWIN32_LEAN_AND_MEAN -I. -I.. ^
    win_main.cpp ^
    win_network_layer.cpp ^
    ..\mc_server.cpp ^
    ..\packet_codec.cpp ^
    -lws2_32 ^
    -o %EXE_NAME%

if %ERRORLEVEL% NEQ 0 ( echo [ERROR] Build failed! & popd & pause & exit /b 1 )
popd

echo [OK] Running %EXE_NAME%...
"%~dp0%EXE_NAME%"
pause
