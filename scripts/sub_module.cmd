@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Run from the repository root regardless of the caller's directory.
cd /d "%~dp0.."

rem .gitmodules is the single source of truth, so this script is idempotent.
rem Some portable Git installations do not add their Unix helper tools to PATH.
for /f "delims=" %%I in ('where git.exe 2^>nul') do if not defined LIMEN_GIT_EXE set "LIMEN_GIT_EXE=%%I"
if defined LIMEN_GIT_EXE (
    for %%I in ("!LIMEN_GIT_EXE!") do set "LIMEN_GIT_ROOT=%%~dpI.."
    if exist "!LIMEN_GIT_ROOT!\usr\bin\basename.exe" set "PATH=!LIMEN_GIT_ROOT!\usr\bin;!PATH!"
)

git submodule sync --recursive
if errorlevel 1 goto :failed

git submodule update --init --recursive
if errorlevel 1 goto :failed

rem 构建环境：下载 stb_truetype 单头文件，已存在则跳过。
set "LIMEN_STB_DIR=%CD%\LimenEngine\vendor\stb_truetype"
if not exist "%LIMEN_STB_DIR%" mkdir "%LIMEN_STB_DIR%"
if not exist "%LIMEN_STB_DIR%\stb_truetype.h" (
    curl -L --fail -o "%LIMEN_STB_DIR%\stb_truetype.h" https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h
    if errorlevel 1 goto :failed
)

echo.
echo Submodules are ready.
exit /b 0

:failed
echo.
echo Failed to initialize submodules or download stb_truetype. Check the Git installation and network access.
exit /b 1
