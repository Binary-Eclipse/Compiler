@echo off
rem Puts the MSYS2 UCRT64 tools (gcc, make, flex, bison) on PATH if MSYS2 is installed.
rem Called by the other scripts; deliberately has no SETLOCAL so PATH changes reach the caller.
if not defined SHAKIL_MSYS2 (
  for %%D in ("C:\msys64" "D:\msys64" "%USERPROFILE%\msys64" "%LOCALAPPDATA%\msys64") do (
    if exist "%%~D\usr\bin\bash.exe" if not defined SHAKIL_MSYS2 set "SHAKIL_MSYS2=%%~D"
  )
)
if not defined SHAKIL_MSYS2 set "SHAKIL_MSYS2=C:\msys64"
if exist "%SHAKIL_MSYS2%\ucrt64\bin\gcc.exe" set "PATH=%SHAKIL_MSYS2%\ucrt64\bin;%SHAKIL_MSYS2%\usr\bin;%PATH%"
exit /b 0
