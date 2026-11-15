@echo off
rem Builds ShakilScript (shakilscript.exe). If gcc/make/flex/bison are missing it runs the
rem first-time setup (setup_windows.ps1) which installs them through MSYS2.
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.." || exit /b 1
call "%~dp0env.bat"
call :check
if "!MISSING!"=="1" (
  echo [ShakilScript] Missing build tools:!MISSING_LIST!
  echo [ShakilScript] Running first-time setup. This installs MSYS2 + gcc, make, flex, bison.
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup_windows.ps1"
  if errorlevel 1 (
    echo.
    echo [ShakilScript] Automatic setup failed. See "Troubleshooting" in README.md.
    exit /b 1
  )
  call "%~dp0env.bat"
  call :check
  if "!MISSING!"=="1" (
    echo [ShakilScript] Still missing:!MISSING_LIST!  -- see "Troubleshooting" in README.md.
    exit /b 1
  )
)
make --no-print-directory
if errorlevel 1 (
  echo.
  echo [ShakilScript] Build FAILED. Read the compiler/Bison/Flex message above.
  exit /b 1
)
exit /b 0

:check
set "MISSING=0"
set "MISSING_LIST="
for %%T in (gcc make flex bison) do (
  where %%T >nul 2>nul
  if errorlevel 1 (
    set "MISSING=1"
    set "MISSING_LIST=!MISSING_LIST! %%T"
  )
)
exit /b 0
