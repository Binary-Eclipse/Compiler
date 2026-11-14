@echo off
rem Run a ShakilScript program:  run_windows.bat program.shakil
rem You can also drag a .shakil file onto this file. Builds automatically if needed.
call "%~dp0scripts\run.bat" %*
set "RC=%ERRORLEVEL%"
echo %cmdcmdline% | find /i "%~nx0" >nul
if not errorlevel 1 pause
exit /b %RC%
