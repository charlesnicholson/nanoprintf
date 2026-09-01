@echo off
rem envy-managed schema "4"
rem setlocal (plain, not delayed -- a product path with '!' must survive): without it PATH
rem grows a copy per invocation and a sibling inherits our product path, re-running us forever.
setlocal
rem The trailing dot keeps %~dp0's own backslash off the closing quote.
set "PATH=%~dp0.;%PATH%"
rem Empty under '@envy root "false"'; the caller's value stands.
set "ENVY_PROJECT_ROOT_HOP=.."
if defined ENVY_PROJECT_ROOT_HOP (
    for %%I in ("%~dp0%ENVY_PROJECT_ROOT_HOP%") do set "ENVY_PROJECT_ROOT=%%~fI"
)
rem The sibling launcher injects --project, so envy resolves the project this script was
rem deployed into, not one rediscovered from the caller's CWD.
rem Cleared first: `for /f` sets nothing on empty output, so the guard below would otherwise
rem pass on a value inherited from an ancestor product script and run that payload.
set "ENVY_PRODUCT_PATH="
for /f "delims=" %%i in ('call "%~dp0envy.bat" product "python"') do set "ENVY_PRODUCT_PATH=%%i"
if not defined ENVY_PRODUCT_PATH (
    echo envy: failed to resolve product 'python' 1>&2
    exit /b 1
)
call "%ENVY_PRODUCT_PATH%" %*
exit /b %ERRORLEVEL%
