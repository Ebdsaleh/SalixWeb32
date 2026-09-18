@echo off
setlocal

for %%I in ("%~dp0..") do set "SALIX_ROOT=%%~fI"
set "PROJECT=%SALIX_ROOT%\build\secure_transport\SalixSecureTransport.vcxproj"
set "OUTPUT=%SALIX_ROOT%\build\secure_transport\bin\Release\SalixSecureTransport.dll"

echo.
echo ============================================================
echo  SalixSecureTransport ABI Test Provider
echo ============================================================
echo Project : %PROJECT%
echo Target  : Win32 / Release / v141_xp
echo.

where msbuild >nul 2>nul
if errorlevel 1 (
    echo ERROR: MSBuild was not found in this command prompt.
    echo Run this script from a Visual Studio Developer Command Prompt.
    exit /b 1
)

msbuild "%PROJECT%" /m /t:Rebuild /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 (
    echo.
    echo ERROR: Provider build failed.
    echo If MSBuild reports that v141_xp is missing, install the VS 2017 XP
    echo platform toolset before continuing. Do not substitute a newer
    echo non-XP toolset for the Server 2003 compatibility test.
    exit /b 1
)

if not exist "%OUTPUT%" (
    echo ERROR: Build succeeded but the expected DLL was not found:
    echo %OUTPUT%
    exit /b 1
)

echo.
echo Built:
echo %OUTPUT%
echo.

where dumpbin >nul 2>nul
if errorlevel 1 (
    echo WARNING: dumpbin was not found; skipping export/dependency inspection.
) else (
    echo ---- exported ABI ----
    dumpbin /exports "%OUTPUT%"
    echo.
    echo ---- direct dependencies ----
    dumpbin /dependents "%OUTPUT%"
)

echo.
echo IMPORTANT:
echo This ABI-test DLL intentionally advertises ZERO TLS capabilities.
echo A successful P4 load must still report real content as BLOCKED.
echo.
exit /b 0
