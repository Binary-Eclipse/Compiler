@echo off
setlocal
cd /d "%~dp0"
where make >nul 2>nul
if errorlevel 1 (
  echo Make was not found. Open the MSYS2 UCRT64 terminal and run: make
  pause
  exit /b 1
)
make
if errorlevel 1 (
  echo Build failed. Read the compiler error above.
  pause
  exit /b 1
)
echo.
echo Build completed. Run:  shakilscript.exe examples\01_hello.ss
echo Syntax check only:    shakilscript.exe --check examples\01_hello.ss
pause
