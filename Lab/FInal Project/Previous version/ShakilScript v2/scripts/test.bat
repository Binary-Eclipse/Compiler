@echo off
rem Builds ShakilScript and runs the whole test suite.
setlocal EnableExtensions
call "%~dp0build.bat"
if errorlevel 1 exit /b 1
cd /d "%~dp0.."
call "%~dp0env.bat"
sh tests/run_tests.sh ./shakilscript.exe
exit /b %ERRORLEVEL%
