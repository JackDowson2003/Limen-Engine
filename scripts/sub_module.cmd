@echo off
setlocal EnableExtensions DisableDelayedExpansion

pushd "%~dp0.."
if errorlevel 1 (
    echo Error: Could not enter the repository root. 1>&2
    exit /b 1
)

for %%I in (git.exe) do set "LIMEN_GIT_EXE=%%~$PATH:I"
if not defined LIMEN_GIT_EXE (
    echo Error: Git was not found in PATH. 1>&2
    goto :failed
)

for %%I in (cmake.exe) do set "LIMEN_CMAKE_EXE=%%~$PATH:I"
if not defined LIMEN_CMAKE_EXE (
    echo Error: CMake was not found in PATH. 1>&2
    goto :failed
)

rem Git for Windows 的便携发行版有时未将 Unix 辅助程序加入 PATH。
for %%I in ("%LIMEN_GIT_EXE%") do set "LIMEN_GIT_ROOT=%%~dpI.."
if exist "%LIMEN_GIT_ROOT%\usr\bin\basename.exe" set "PATH=%LIMEN_GIT_ROOT%\usr\bin;%PATH%"

"%LIMEN_GIT_EXE%" submodule update --init --recursive
if errorlevel 1 (
    echo Error: Git could not initialize the tracked submodules. 1>&2
    goto :failed
)

"%LIMEN_CMAKE_EXE%" -P "%~dp0prepare-vendor-headers.cmake"
if errorlevel 1 (
    echo Error: CMake could not prepare the vendor headers. 1>&2
    goto :failed
)

popd
echo Submodules and vendor headers are ready.
exit /b 0

:failed
popd
exit /b 1
