@echo off
REM envy-managed bootstrap script - do not edit
setlocal EnableDelayedExpansion

set "ENVY_DEFAULT_MIRROR=https://github.com/envy-package-manager/envy/releases/download"
set "ENVY_LATEST_URL=https://github.com/envy-package-manager/envy/releases/latest"
set "ENVY_ENV_MIRROR="
if defined ENVY_MIRROR set "ENVY_ENV_MIRROR=%ENVY_MIRROR%"
set "ENVY_FALLBACK_VERSION=0.2.3"
set "ENVY_MIN_DIRECTIVE_VERSION=0.2.0"

set "ENVY_MANIFEST="
set "ENVY_CANDIDATE="
set "ENVY_DIR=%~dp0"
if "!ENVY_DIR:~-1!"=="\" set "ENVY_DIR=!ENVY_DIR:~0,-1!"
:findloop
if exist "!ENVY_DIR!\envy.lua" (
    call :read_root "!ENVY_DIR!\envy.lua"
    if "!ENVY_IS_ROOT!"=="true" (
        set "ENVY_MANIFEST=!ENVY_DIR!\envy.lua"
        goto :found
    ) else (
        set "ENVY_CANDIDATE=!ENVY_DIR!\envy.lua"
    )
)
REM A repo is a hard boundary, matching discover() in src/manifest.cpp. The trailing
REM backslash makes `if exist` test for a directory, as discover()'s is_directory does.
if exist "!ENVY_DIR!\.git\" (
    if defined ENVY_CANDIDATE (
        set "ENVY_MANIFEST=!ENVY_CANDIDATE!"
        goto :found
    )
    echo ERROR: envy.lua not found >&2 & exit /b 1
)
for %%I in ("!ENVY_DIR!\..") do set "ENVY_PARENT=%%~fI"
if "!ENVY_PARENT!"=="!ENVY_DIR!" (
    if defined ENVY_CANDIDATE (
        set "ENVY_MANIFEST=!ENVY_CANDIDATE!"
        goto :found
    )
    echo ERROR: envy.lua not found >&2 & exit /b 1
)
set "ENVY_DIR=!ENVY_PARENT!"
goto :findloop
:found

set "ENVY_VERSION="
set "ENVY_CACHE_LOCAL="
set "ENVY_CACHE_MODE="
set "ENVY_STATE_DIR_REL="
set "ENVY_MANIFEST_MIRROR="
set "ENVY_SUMS_PIN="

REM Header only, stopping at the first code line, matching parse_envy_meta. No line cap. No
REM `delims=` override (default space+tab keeps a tab-indented comment's tab out of %%a);
REM `eol=` clears cmd's `;` comment char, and is last because it eats the next character.
for /f "usebackq tokens=1,2,3,* eol=" %%a in ("!ENVY_MANIFEST!") do (
    set "ENVY_TOK=%%a"
    if not "!ENVY_TOK:~0,2!"=="--" goto :done_parse
    if "%%a"=="--" if "%%b"=="@envy" (
        set "ENVY_KEY=%%c"
        set "ENVY_RAW=%%d"
        if defined ENVY_RAW (
            call :quoted_value
            if defined ENVY_VAL (
                if "!ENVY_KEY!"=="version" set "ENVY_VERSION=!ENVY_VAL!"
                if "!ENVY_KEY!"=="cache-local" set "ENVY_CACHE_LOCAL=!ENVY_VAL!"
                if "!ENVY_KEY!"=="cache-mode" set "ENVY_CACHE_MODE=!ENVY_VAL!"
                if "!ENVY_KEY!"=="state-dir" set "ENVY_STATE_DIR_REL=!ENVY_VAL!"
                if "!ENVY_KEY!"=="mirror" set "ENVY_MANIFEST_MIRROR=!ENVY_VAL!"
                if "!ENVY_KEY!"=="sha256sums" set "ENVY_SUMS_PIN=!ENVY_VAL!"
            )
        )
    )
)
:done_parse

REM Captured before the resolution chain overwrites it: a pin is meaningless against a
REM resolved version.
set "ENVY_PINNED_VERSION=!ENVY_VERSION!"

REM Fail closed before any network: a pin that silently stops verifying is worse than none.
if defined ENVY_SUMS_PIN if not defined ENVY_PINNED_VERSION (
    echo ERROR: '@envy sha256sums' requires '@envy version' in !ENVY_MANIFEST! >&2
    exit /b 1
)

REM Precedence: ENVY_MIRROR env > @envy mirror > upstream, matching src/reexec.cpp.
REM ENVY_DEFAULT_MIRROR is always envy's own release URL, never a copy of the directive's.
if defined ENVY_ENV_MIRROR (
    set "ENVY_MIRROR=!ENVY_ENV_MIRROR!"
) else if defined ENVY_MANIFEST_MIRROR (
    set "ENVY_MIRROR=!ENVY_MANIFEST_MIRROR!"
) else (
    set "ENVY_MIRROR=!ENVY_DEFAULT_MIRROR!"
)

REM A trailing slash makes ".../releases//v1.2.3/...", a distinct nonexistent s3:// key.
:striptrail
if "!ENVY_MIRROR:~-1!"=="/" (
    set "ENVY_MIRROR=!ENVY_MIRROR:~0,-1!"
    goto :striptrail
)

set "ENVY_MIRROR_IS_S3="
if /i "!ENVY_MIRROR:~0,5!"=="s3://" set "ENVY_MIRROR_IS_S3=1"

REM Bare `aws`, not `aws.exe`: PATHEXT also resolves the aws.cmd/aws.bat shims. The
REM curl.exe/tar.exe probes below name the exe deliberately, to stay policy-proof.
if not defined ENVY_MIRROR_IS_S3 goto :mirror_ok
where /q aws && goto :mirror_ok
echo ERROR: mirror "!ENVY_MIRROR!" is an s3:// URI but the aws CLI was not found on PATH. >&2
echo        Install AWS CLI v2, or use an https:// mirror. >&2
exit /b 1
:mirror_ok

set "ENVY_MANIFEST_DIR="
for %%I in ("!ENVY_MANIFEST!") do set "ENVY_MANIFEST_DIR=%%~dpI"
if "!ENVY_MANIFEST_DIR:~-1!"=="\" set "ENVY_MANIFEST_DIR=!ENVY_MANIFEST_DIR:~0,-1!"

REM Cleared ahead of the tiers: setlocal copies the parent environment, so an exported
REM ENVY_MODE would reach the candidate selection.
set "ENVY_MODE="
set "ENVY_SHARED_CACHE="
set "ENVY_MODE_FROM_MARKER="

REM Tiers, in order, matching resolve_cache_root() in src/cache.cpp. No expansion of any
REM kind, so this script and the binary have no grammar to disagree about. Flat `goto`, not
REM if/else: in parens cmd re-parses REM lines and a stray `(`/`&`/`>` breaks the block.
if not defined ENVY_CACHE_ROOT goto :resolve_project_cache
set "ENVY_CACHE=!ENVY_CACHE_ROOT!"
call :require_absolute
if errorlevel 1 exit /b 1
goto :cache_resolved

:resolve_project_cache
REM Guarded because an unguarded join yields "\envy" -- *defined*, so `if exist` takes the
REM drive root for it.
if defined LOCALAPPDATA set "ENVY_SHARED_CACHE=!LOCALAPPDATA!\envy"

REM @envy state-dir, else the manifest's own directory -- never `.envy`, which sits inside
REM the default local tree `.envy\cache` and would let a cache wipe erase the marker.
set "ENVY_STATE_DIR=!ENVY_MANIFEST_DIR!"
if defined ENVY_STATE_DIR_REL set "ENVY_STATE_DIR=!ENVY_MANIFEST_DIR!\!ENVY_STATE_DIR_REL!"

REM Existence is the whole signal -- no content for three languages to read three ways.
if exist "!ENVY_STATE_DIR!\.envy-cache-local" if exist "!ENVY_STATE_DIR!\.envy-cache-shared" (
    echo ERROR: both .envy-cache-local and .envy-cache-shared exist in !ENVY_STATE_DIR! >&2
    echo        envy never writes both. Delete one. >&2
    exit /b 1
)

REM Naming a tree is asking for it. A marker leaves no directive for the version guard
REM below, so it is remembered separately.
if exist "!ENVY_STATE_DIR!\.envy-cache-local" set "ENVY_MODE=local"
if not defined ENVY_MODE if exist "!ENVY_STATE_DIR!\.envy-cache-shared" set "ENVY_MODE=shared"
if defined ENVY_MODE set "ENVY_MODE_FROM_MARKER=1"
if not defined ENVY_MODE if defined ENVY_CACHE_MODE set "ENVY_MODE=!ENVY_CACHE_MODE!"
if not defined ENVY_MODE if defined ENVY_CACHE_LOCAL set "ENVY_MODE=local"
if not defined ENVY_MODE set "ENVY_MODE=shared"

if "!ENVY_MODE!"=="local" goto :cache_local
REM Shared mode *is* the platform default, so with LOCALAPPDATA unset there is no root.
if defined ENVY_SHARED_CACHE goto :cache_shared_ok
echo ERROR: cannot determine a cache root: LOCALAPPDATA is not set. Set ENVY_CACHE_ROOT, >&2
echo        or give the project a local cache. >&2
exit /b 1
:cache_shared_ok
set "ENVY_CACHE=!ENVY_SHARED_CACHE!"
goto :cache_resolved

:cache_local
REM Anchored to the manifest dir, never the caller's cwd: one manifest, one tree. %%~fI
REM normalizes, matching the binary's lexically_normal/make_preferred.
set "ENVY_CACHE=!ENVY_MANIFEST_DIR!\.envy\cache"
if defined ENVY_CACHE_LOCAL set "ENVY_CACHE=!ENVY_MANIFEST_DIR!\!ENVY_CACHE_LOCAL!"
for %%I in ("!ENVY_CACHE!") do set "ENVY_CACHE=%%~fI"

:cache_resolved

if "!ENVY_VERSION!"=="" (
    set "ENVY_LATEST_FILE=!ENVY_CACHE!\envy\latest"
    if exist "!ENVY_LATEST_FILE!" (
        set /p ENVY_LATEST_VER=<"!ENVY_LATEST_FILE!"
        if defined ENVY_LATEST_VER (
            if exist "!ENVY_CACHE!\envy\!ENVY_LATEST_VER!\envy.exe" set "ENVY_VERSION=!ENVY_LATEST_VER!"
        )
    )
)
if not "!ENVY_VERSION!"=="" goto :version_resolved

REM 'envy mirror-envy' writes `latest` at the mirror root, so a private mirror answers.
set "ENVY_LATEST_TMP=!TEMP!\envy-latest-%RANDOM%%RANDOM%.txt"
set "ENVY_GOT="
if defined ENVY_MIRROR_IS_S3 (
    call aws s3 cp --only-show-errors "!ENVY_MIRROR!/latest" "!ENVY_LATEST_TMP!" >nul 2>&1 && set "ENVY_GOT=1"
) else (
    where /q curl.exe && (curl.exe -fsSL --connect-timeout 10 --max-time 300 "!ENVY_MIRROR!/latest" -o "!ENVY_LATEST_TMP!" >nul 2>&1 && set "ENVY_GOT=1")
)
if not defined ENVY_GOT goto :latest_cleanup
REM Unquoted for /f (a literal, not a filename -- no usebackq), staged so a whitespace-only
REM file leaves ENVY_VERSION empty.
set "ENVY_RAW_VERSION="
set /p ENVY_RAW_VERSION=<"!ENVY_LATEST_TMP!"
for /f "tokens=1" %%v in ("!ENVY_RAW_VERSION!") do set "ENVY_VERSION=%%v"
set "ENVY_VERSION_SRC=!ENVY_MIRROR!/latest"
call :check_version
:latest_cleanup
del "!ENVY_LATEST_TMP!" 2>nul
if not "!ENVY_VERSION!"=="" goto :version_resolved

REM GitHub serves no `latest` object, so fall back to its redirect. Skipped for s3://
REM mirrors, which are never github.
if defined ENVY_MIRROR_IS_S3 goto :version_fallback

REM Parse the tag from the end of the redirect chain, not hop 1 (a repo rename inserts a
REM hop ending in `latest`). To a file behind &&: --fail writes -w output on an HTTP error.
set "ENVY_EFF_TMP=!TEMP!\envy-effective-%RANDOM%%RANDOM%.txt"
set "ENVY_EFFECTIVE="
set "ENVY_TAG="
set "ENVY_GOT="
where /q curl.exe && (curl.exe -fsSL -o nul -w "%%{url_effective}" --connect-timeout 5 --max-time 15 "!ENVY_LATEST_URL!" >"!ENVY_EFF_TMP!" 2>nul && set "ENVY_GOT=1")
REM goto, not `if defined ... set /p`: cmd redirects whether or not the `if` body runs.
if not defined ENVY_GOT goto :effective_cleanup
set /p ENVY_EFFECTIVE=<"!ENVY_EFF_TMP!"
:effective_cleanup
del "!ENVY_EFF_TMP!" 2>nul
if defined ENVY_EFFECTIVE set "ENVY_EFFECTIVE=!ENVY_EFFECTIVE:/=\!"
if defined ENVY_EFFECTIVE for %%a in ("!ENVY_EFFECTIVE!") do set "ENVY_TAG=%%~nxa"
if defined ENVY_TAG set "ENVY_VERSION=!ENVY_TAG!"
if defined ENVY_TAG if "!ENVY_TAG:~0,1!"=="v" set "ENVY_VERSION=!ENVY_TAG:~1!"
REM PowerShell fallback: AllowAutoRedirect defaults on, so ResponseUri ends the chain.
if "!ENVY_VERSION!"=="" (
    for /f "tokens=*" %%u in ('powershell -NoProfile -Command "$ProgressPreference='SilentlyContinue'; try { $resp=[System.Net.WebRequest]::Create('!ENVY_LATEST_URL!').GetResponse(); $u=$resp.ResponseUri.AbsoluteUri; $resp.Close(); ($u -split '/')[-1] -replace '^v','' } catch {}" 2^>nul') do set "ENVY_VERSION=%%u"
)
set "ENVY_VERSION_SRC=!ENVY_LATEST_URL!"
call :check_version

:version_fallback
if "!ENVY_VERSION!"=="" set "ENVY_VERSION=!ENVY_FALLBACK_VERSION!"
:version_resolved

REM An older envy silently ignores the cache directives, resolving the *shared* cache for a
REM manifest asking for a hermetic tree and exiting 0. Refuse before downloading it. 0.0.0
REM is a dev build, let through as in reexec_should().
set "ENVY_USES_NEW_DIRECTIVES="
if defined ENVY_CACHE_LOCAL set "ENVY_USES_NEW_DIRECTIVES=1"
if defined ENVY_CACHE_MODE set "ENVY_USES_NEW_DIRECTIVES=1"
if defined ENVY_STATE_DIR_REL set "ENVY_USES_NEW_DIRECTIVES=1"
REM A recorded mode counts as much as a declared one.
if defined ENVY_MODE_FROM_MARKER set "ENVY_USES_NEW_DIRECTIVES=1"
REM The errorlevel test lives inside the guard: outside it, it read whatever the preceding
REM `set`/`if` left behind.
if defined ENVY_USES_NEW_DIRECTIVES if not "!ENVY_VERSION!"=="0.0.0" (
    call :guard_directive_version
    if errorlevel 1 exit /b 1
)

REM Regular and non-empty: bare `if exist` accepts a directory or a zero-length file.
set "ENVY_BIN=!ENVY_CACHE!\envy\!ENVY_VERSION!\envy.exe"
set "ENVY_BIN_OK="
if exist "!ENVY_BIN!" if not exist "!ENVY_BIN!\" for %%I in ("!ENVY_BIN!") do if not "%%~zI"=="0" set "ENVY_BIN_OK=1"
if defined ENVY_BIN_OK goto :run

REM A local tree borrows the user's own copy. Above the "Downloading envy" banner, so a hit
REM never announces a download.
if not "!ENVY_MODE!"=="local" goto :no_shared_probe
if defined ENVY_SUMS_PIN goto :no_shared_probe
if not defined ENVY_SHARED_CACHE goto :no_shared_probe
set "ENVY_SHARED_BIN=!ENVY_SHARED_CACHE!\envy\!ENVY_VERSION!\envy.exe"
set "ENVY_BIN_OK="
if exist "!ENVY_SHARED_BIN!" if not exist "!ENVY_SHARED_BIN!\" for %%I in ("!ENVY_SHARED_BIN!") do if not "%%~zI"=="0" set "ENVY_BIN_OK=1"
if not defined ENVY_BIN_OK goto :no_shared_probe
set "ENVY_BIN=!ENVY_SHARED_BIN!"
goto :run
:no_shared_probe

set "ENVY_ARCH=x86_64"
reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v PROCESSOR_ARCHITECTURE 2>nul | findstr /i "ARM64" >nul 2>&1 && set "ENVY_ARCH=arm64"

echo Downloading envy !ENVY_VERSION!... >&2
set "ENVY_ARCHIVE=envy-windows-!ENVY_ARCH!.zip"
set "ENVY_URL=!ENVY_MIRROR!/v!ENVY_VERSION!/!ENVY_ARCHIVE!"
REM Escape single quotes for PowerShell.
set "ENVY_SAFE_URL=!ENVY_URL:'=''!"
REM Atomic mkdir: %RANDOM% collides across concurrent bootstraps, mkdir does not.
set /a ENVY_TEMP_TRIES=0
:mktemp
set "ENVY_TEMP_DIR=!TEMP!\envy-%RANDOM%%RANDOM%"
mkdir "!ENVY_TEMP_DIR!" 2>nul && goto :gottemp
set /a ENVY_TEMP_TRIES+=1
if !ENVY_TEMP_TRIES! LSS 10 goto :mktemp
echo ERROR: Could not create a temp directory under !TEMP! >&2 & exit /b 1
:gottemp
set "ENVY_TEMP_ZIP=!ENVY_TEMP_DIR!.zip"

REM Download: prefer native curl.exe (policy-resistant), fall back to PowerShell.
set "ENVY_OK="
if defined ENVY_MIRROR_IS_S3 goto :dl_s3
goto :dl_http

:dl_s3
REM `call` so an aws .bat/.cmd shim returns control here with ERRORLEVEL intact. To a file,
REM never piped: cmd reads ERRORLEVEL from a pipe's right side, and tar exits 0 on nothing.
call aws s3 cp --only-show-errors "!ENVY_URL!" "!ENVY_TEMP_ZIP!" && set "ENVY_OK=1"
goto :dl_done

:dl_http
where /q curl.exe && (curl.exe -fsSL "!ENVY_URL!" -o "!ENVY_TEMP_ZIP!" && set "ENVY_OK=1")
if not defined ENVY_OK (
    powershell -NoProfile -Command "$ProgressPreference='SilentlyContinue'; Invoke-WebRequest -Uri '!ENVY_SAFE_URL!' -OutFile '!ENVY_TEMP_ZIP!' -UseBasicParsing" && set "ENVY_OK=1"
)

:dl_done
if not defined ENVY_OK (echo ERROR: Failed to download envy from !ENVY_URL! >&2 & rmdir /s /q "!ENVY_TEMP_DIR!" 2>nul & del "!ENVY_TEMP_ZIP!" 2>nul & exit /b 1)

REM Attest before extracting: a hostile mirror must not choose paths we run envy from.
if not defined ENVY_SUMS_PIN goto :attest_done

set "ENVY_SUMS_URL=!ENVY_MIRROR!/v!ENVY_VERSION!/SHA256SUMS"
set "ENVY_SAFE_SUMS_URL=!ENVY_SUMS_URL:'=''!"
set "ENVY_SUMS_FILE=!ENVY_TEMP_DIR!\SHA256SUMS"
set "ENVY_OK="
if defined ENVY_MIRROR_IS_S3 (
    call aws s3 cp --only-show-errors "!ENVY_SUMS_URL!" "!ENVY_SUMS_FILE!" && set "ENVY_OK=1"
) else (
    where /q curl.exe && (curl.exe -fsSL "!ENVY_SUMS_URL!" -o "!ENVY_SUMS_FILE!" && set "ENVY_OK=1")
    if not defined ENVY_OK (
        powershell -NoProfile -Command "$ProgressPreference='SilentlyContinue'; Invoke-WebRequest -Uri '!ENVY_SAFE_SUMS_URL!' -OutFile '!ENVY_SUMS_FILE!' -UseBasicParsing" && set "ENVY_OK=1"
    )
)
if not defined ENVY_OK (echo ERROR: Failed to download !ENVY_SUMS_URL! >&2 & rmdir /s /q "!ENVY_TEMP_DIR!" 2>nul & del "!ENVY_TEMP_ZIP!" 2>nul & exit /b 1)

REM Anchor the chain on the manifest's pin before trusting anything the sums file says.
set "ENVY_HASH_FILE=!ENVY_SUMS_FILE!"
call :sha256
if not defined ENVY_HASH_OUT (echo ERROR: could not compute a SHA256 of !ENVY_SUMS_FILE! >&2 & rmdir /s /q "!ENVY_TEMP_DIR!" 2>nul & del "!ENVY_TEMP_ZIP!" 2>nul & exit /b 1)
if /i not "!ENVY_HASH_OUT!"=="!ENVY_SUMS_PIN!" (
    echo ERROR: SHA256SUMS does not match the pinned '@envy sha256sums': >&2
    echo        expected !ENVY_SUMS_PIN! >&2
    echo        got      !ENVY_HASH_OUT! >&2
    echo        The mirror is serving a different release manifest than !ENVY_MANIFEST! pinned. >&2
    echo        Update the pin deliberately; do not remove it. >&2
    rmdir /s /q "!ENVY_TEMP_DIR!" 2>nul & del "!ENVY_TEMP_ZIP!" 2>nul & exit /b 1
)

REM Exact archive: hash alone accepts any platform, a prefix name a longer sibling.
set "ENVY_WANT="
for /f "usebackq tokens=1,2" %%h in ("!ENVY_SUMS_FILE!") do (
    set "ENVY_NAME=%%i"
    if "!ENVY_NAME:~0,1!"=="*" set "ENVY_NAME=!ENVY_NAME:~1!"
    if /i "!ENVY_NAME!"=="!ENVY_ARCHIVE!" if not defined ENVY_WANT set "ENVY_WANT=%%h"
)
if not defined ENVY_WANT (echo ERROR: SHA256SUMS lists no entry for !ENVY_ARCHIVE! >&2 & rmdir /s /q "!ENVY_TEMP_DIR!" 2>nul & del "!ENVY_TEMP_ZIP!" 2>nul & exit /b 1)

set "ENVY_HASH_FILE=!ENVY_TEMP_ZIP!"
call :sha256
if not defined ENVY_HASH_OUT (echo ERROR: could not compute a SHA256 of !ENVY_TEMP_ZIP! >&2 & rmdir /s /q "!ENVY_TEMP_DIR!" 2>nul & del "!ENVY_TEMP_ZIP!" 2>nul & exit /b 1)
if /i not "!ENVY_HASH_OUT!"=="!ENVY_WANT!" (
    echo ERROR: !ENVY_ARCHIVE! failed attestation: >&2
    echo        SHA256SUMS says !ENVY_WANT! >&2
    echo        downloaded      !ENVY_HASH_OUT! >&2
    rmdir /s /q "!ENVY_TEMP_DIR!" 2>nul & del "!ENVY_TEMP_ZIP!" 2>nul & exit /b 1
)
:attest_done

REM Extract: prefer native tar.exe (bsdtar reads zip), fall back to Expand-Archive.
set "ENVY_OK="
where /q tar.exe && (tar.exe -xf "!ENVY_TEMP_ZIP!" -C "!ENVY_TEMP_DIR!" && set "ENVY_OK=1")
if not defined ENVY_OK (
    powershell -NoProfile -Command "$ProgressPreference='SilentlyContinue'; Expand-Archive -Path '!ENVY_TEMP_ZIP!' -DestinationPath '!ENVY_TEMP_DIR!' -Force" && set "ENVY_OK=1"
)
if not defined ENVY_OK (echo ERROR: Failed to extract envy >&2 & rmdir /s /q "!ENVY_TEMP_DIR!" 2>nul & del "!ENVY_TEMP_ZIP!" 2>nul & exit /b 1)
del "!ENVY_TEMP_ZIP!" 2>nul
REM tar succeeds on an empty archive; without this, :run reports a missing path instead.
if not exist "!ENVY_TEMP_DIR!\envy.exe" (echo ERROR: archive from !ENVY_URL! contained no envy binary >&2 & rmdir /s /q "!ENVY_TEMP_DIR!" 2>nul & exit /b 1)
set "ENVY_BIN=!ENVY_TEMP_DIR!\envy.exe"
goto :run

REM :read_root -- ENVY_IS_ROOT out, manifest path in %1. Header-only scan, same rules as
REM the one above. A subroutine so the early-exit `goto` lands at a scope's top level
REM instead of tearing out of the caller's `if exist (...)` block. Reached only by `call`.
:read_root
set "ENVY_IS_ROOT=true"
for /f "usebackq tokens=1,2,3,4 eol=" %%a in ("%~1") do (
    set "ENVY_TOK=%%a"
    if not "!ENVY_TOK:~0,2!"=="--" goto :read_root_done
    if "%%a"=="--" if "%%b"=="@envy" if "%%c"=="root" (
        set "ENVY_VAL=%%d"
        set "ENVY_VAL=!ENVY_VAL:"=!"
        if "!ENVY_VAL!"=="false" set "ENVY_IS_ROOT=false"
    )
)
:read_root_done
exit /b 0

REM :quoted_value -- ENVY_RAW in (a directive's `*` remainder, opening quote onward),
REM ENVY_VAL out. `for /f` skips leading delimiters, so with `"` as delimiter the value is
REM token 1 and ends at the closing quote, matching parse_directive_line(). A `\"` inside a
REM value still splits early -- see docs/envy-init.md. Options are caret-escaped and
REM unquoted: `delims="` cannot be written inside quotes. Reached only by `call`.
:quoted_value
set "ENVY_VAL="
for /f tokens^=1^ delims^=^" %%v in ("!ENVY_RAW!") do set "ENVY_VAL=%%v"
if defined ENVY_VAL set "ENVY_VAL=!ENVY_VAL:\\=\!"
exit /b 0

REM :require_absolute -- ENVY_CACHE in; errorlevel 1 when not absolute. Rejected, not
REM absolutized: the binary anchors a relative override to its own cwd. `call` only.
:require_absolute
if "!ENVY_CACHE:~0,2!"=="\\" exit /b 0
if "!ENVY_CACHE:~0,2!"=="//" exit /b 0
if "!ENVY_CACHE:~1,2!"==":\" exit /b 0
if "!ENVY_CACHE:~1,2!"==":/" exit /b 0
echo ERROR: ENVY_CACHE_ROOT must be an absolute path: !ENVY_CACHE! >&2
exit /b 1

REM :guard_directive_version -- errorlevel 1 when the resolved envy predates the cache
REM directives this manifest uses. Field-wise integer compare: 0.10.0 > 0.2.0. `call` only.
:guard_directive_version
echo(!ENVY_MIN_DIRECTIVE_VERSION!|findstr /r /x /c:"[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*" >nul 2>&1
if errorlevel 1 exit /b 0
echo(!ENVY_VERSION!|findstr /r /x /c:"[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*" >nul 2>&1
if errorlevel 1 exit /b 0
for /f "tokens=1-3 delims=." %%x in ("!ENVY_VERSION!") do (
    set "ENVY_A1=%%x" & set "ENVY_A2=%%y" & set "ENVY_A3=%%z"
)
for /f "tokens=1-3 delims=." %%x in ("!ENVY_MIN_DIRECTIVE_VERSION!") do (
    set "ENVY_B1=%%x" & set "ENVY_B2=%%y" & set "ENVY_B3=%%z"
)
set "ENVY_OLD="
if !ENVY_A1! LSS !ENVY_B1! set "ENVY_OLD=1"
if not defined ENVY_OLD if !ENVY_A1! EQU !ENVY_B1! if !ENVY_A2! LSS !ENVY_B2! set "ENVY_OLD=1"
if not defined ENVY_OLD if !ENVY_A1! EQU !ENVY_B1! if !ENVY_A2! EQU !ENVY_B2! if !ENVY_A3! LSS !ENVY_B3! set "ENVY_OLD=1"
if not defined ENVY_OLD exit /b 0
echo ERROR: !ENVY_MANIFEST! uses '@envy cache-local'/'cache-mode'/'state-dir', added in envy >&2
echo        !ENVY_MIN_DIRECTIVE_VERSION!, but resolves envy !ENVY_VERSION!. That envy ignores them >&2
echo        and would silently use the shared cache. Raise or remove the '@envy version' pin. >&2
exit /b 1

REM :check_version -- ENVY_VERSION and ENVY_VERSION_SRC in; clears ENVY_VERSION unless it
REM is MAJOR.MINOR.PATCH, deferring to the next tier. Delayed-expanded, so an `&` in a
REM mirror's `latest` is data, not a command.
:check_version
if not defined ENVY_VERSION exit /b 0
echo(!ENVY_VERSION!|findstr /r /x /c:"[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*" >nul 2>&1
if not errorlevel 1 exit /b 0
echo WARNING: ignoring implausible envy version '!ENVY_VERSION!' from !ENVY_VERSION_SRC! >&2
set "ENVY_VERSION="
exit /b 0

REM :sha256 -- ENVY_HASH_FILE in, ENVY_HASH_OUT out; empty if no hasher produced a 64-digit
REM digest. certutil first (System32, survives PowerShell lockdowns), but it is a known
REM LOLBin some environments block -- hence the Get-FileHash fallback. `call` only.
:sha256
set "ENVY_HASH_OUT="
where /q certutil.exe && (
    for /f "usebackq skip=1 tokens=*" %%h in (`certutil.exe -hashfile "!ENVY_HASH_FILE!" SHA256 2^>nul`) do (
        if not defined ENVY_HASH_OUT set "ENVY_HASH_OUT=%%h"
    )
)
REM Windows 7/8 certutil grouped the digest into space-separated byte pairs; Win10+ not.
if defined ENVY_HASH_OUT set "ENVY_HASH_OUT=!ENVY_HASH_OUT: =!"
call :sha256_len_ok
if not defined ENVY_HASH_OUT (
    set "ENVY_SAFE_HASH_FILE=!ENVY_HASH_FILE:'=''!"
    for /f "usebackq tokens=*" %%h in (`powershell -NoProfile -Command "try{(Get-FileHash -LiteralPath '!ENVY_SAFE_HASH_FILE!' -Algorithm SHA256).Hash}catch{}" 2^>nul`) do set "ENVY_HASH_OUT=%%h"
    call :sha256_len_ok
)
exit /b 0

REM Not exactly 64 chars: certutil writes its status line to stdout, and a localized or
REM error line would be compared against a pin as a digest.
:sha256_len_ok
if not defined ENVY_HASH_OUT exit /b 0
if "!ENVY_HASH_OUT:~63,1!"=="" set "ENVY_HASH_OUT="
if defined ENVY_HASH_OUT if not "!ENVY_HASH_OUT:~64!"=="" set "ENVY_HASH_OUT="
exit /b 0

REM --project ahead of the caller's argv: this script belongs to one project; take_last
REM means a hand-typed --project wins. The dot keeps %~dp0's backslash off the quote.
REM envy sync may rewrite this script; single line ensures cmd.exe never reads past here.
:run
"!ENVY_BIN!" --project "%~dp0." %* & exit /b !ERRORLEVEL!
