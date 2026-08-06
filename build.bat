@echo off
setlocal
cd /d "%~dp0"

REM NPF_DOCTEST_H opts out of envy, so the interpreter comes from PATH: the shim re-execs
REM envy. Otherwise the shim resolves (and installs, if needed) the interpreter envy pins.
if defined NPF_DOCTEST_H (
  python build.py %*
) else (
  call bin\python3.bat build.py %*
)
exit /b %ERRORLEVEL%
