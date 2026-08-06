@echo off
setlocal
cd /d "%~dp0"

REM NPF_DOCTEST_H names a host doctest.h and opts out of envy: the interpreter has to come
REM from PATH too, since the shim below is what re-execs envy. See README "Building
REM without envy". Otherwise the shim resolves (and installs, if needed) the interpreter
REM envy pins.
if defined NPF_DOCTEST_H (
  python build.py %*
) else (
  call bin\python3.bat build.py %*
)
exit /b %ERRORLEVEL%
