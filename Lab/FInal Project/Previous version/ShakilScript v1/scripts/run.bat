@echo off
rem Builds ShakilScript if needed, then runs one program:   scripts\run.bat path\to\program.shakil
setlocal EnableExtensions
if "%~1"=="" (
  echo Usage: scripts\run.bat ^<program.shakil^>
  exit /b 1
)
set "SRC=%~f1"
if not exist "%SRC%" (
  echo File not found: "%SRC%"
  exit /b 1
)
call "%~dp0build.bat"
if errorlevel 1 exit /b 1
echo --- running %~nx1 ---
"%~dp0..\shakilscript.exe" "%SRC%"
set "RC=%ERRORLEVEL%"
echo.
echo --- finished ^(exit code %RC%^) ---
exit /b %RC%
