@echo off
setlocal
cd /d "%~dp0"

REM The shim resolves (and installs, if needed) the interpreter envy pins.
call bin\python3.bat build.py %*
exit /b %ERRORLEVEL%
