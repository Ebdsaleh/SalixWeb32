@echo off
setlocal

for %%I in ("%~dp0..") do set "SALIX_ROOT=%%~fI"
set "EXTENSION_MANIFEST=%SALIX_ROOT%\tools\librewolf_chat_relay_extension\manifest.json"

echo.
echo ============================================================
echo  SalixWeb32 LibreWolf Chat Relay Setup
echo ============================================================
echo.
echo This relay no longer uses Selenium, GeckoDriver, or Marionette.
echo.
echo 1. Start LibreWolf normally with your usual profile.
echo 2. Open:
echo      about:debugging#/runtime/this-firefox
echo 3. Click "Load Temporary Add-on..."
echo 4. Select:
echo      %EXTENSION_MANIFEST%
echo 5. Open the ChatGPT thread you want SalixWeb32 to use.
echo 6. In a terminal run:
echo      python tools\salix_chat_session.py
echo 7. In another terminal run:
echo      python tools\salix_bridge.py --host 0.0.0.0 --port 8765
echo.
echo The temporary extension remains loaded until LibreWolf restarts.
echo.

if exist "%ProgramFiles%\LibreWolf\librewolf.exe" (
    echo Opening LibreWolf about:debugging...
    start "" "%ProgramFiles%\LibreWolf\librewolf.exe" "about:debugging#/runtime/this-firefox"
) else if exist "%ProgramFiles(x86)%\LibreWolf\librewolf.exe" (
    echo Opening LibreWolf about:debugging...
    start "" "%ProgramFiles(x86)%\LibreWolf\librewolf.exe" "about:debugging#/runtime/this-firefox"
) else (
    echo LibreWolf was not found in the standard Program Files locations.
    echo Open about:debugging#/runtime/this-firefox manually in LibreWolf.
)

echo.
exit /b 0
