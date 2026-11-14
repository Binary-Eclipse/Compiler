@echo off
rem Double-click to build ShakilScript. Installs the needed tools first if they are missing.
setlocal
cd /d "%~dp0"
call scripts\build.bat
if errorlevel 1 (
  pause
  exit /b 1
)
echo.
echo Build completed: shakilscript.exe
echo Run a program with:   run_windows.bat examples\first_program.ss   ^(or press F5 in VS Code^)
pause
