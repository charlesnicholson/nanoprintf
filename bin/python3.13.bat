@echo off
rem envy-managed schema "1"
for /f "delims=" %%i in ('call "%~dp0envy.bat" product "python3.13"') do set "PRODUCT_PATH=%%i"
if not defined PRODUCT_PATH (
    echo envy: failed to resolve product 'python3.13' 1>&2
    exit /b 1
)
call "%PRODUCT_PATH%" %*
exit /b %ERRORLEVEL%
