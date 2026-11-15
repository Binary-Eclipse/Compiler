
@echo off
title ShakilScript Runner

cd /d "%~dp0"

if not exist "shakilscript.exe" (
    echo ERROR: shakilscript.exe not found!
    pause
    exit /b 1
)

if "%~1"=="" goto ask

set "PROGRAM=%~1"
goto run

:ask
echo.
echo ==================================
echo       SHAKILSCRIPT RUNNER
echo ==================================
echo.
set /p "PROGRAM=Enter program path (example: programs\hello.ss): "

:run
if not exist "%PROGRAM%" (
    echo.
    echo ERROR: Program file not found!
    pause
    exit /b 1
)

echo.
echo Running: %PROGRAM%
echo ==================================
echo.

shakilscript.exe "%PROGRAM%"

echo.
echo ==================================
echo Program finished.
pause
