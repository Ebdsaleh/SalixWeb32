@echo off
setlocal

for %%I in ("%~dp0..") do set "SALIX_ROOT=%%~fI"

echo.
echo ============================================================
echo  SalixWeb32 LibreWolf Chat Session Setup
echo ============================================================
echo.
echo Installing/updating Selenium into the current Python environment...
echo.

python -m pip install --upgrade -r "%SALIX_ROOT%\tools\requirements-chat-session.txt"
if errorlevel 1 (
    echo.
    echo ERROR: Selenium installation failed.
    exit /b 1
)

echo.
echo Setup complete.
echo.
echo Start the browser worker with:
echo   python tools\salix_chat_session.py
echo.
exit /b 0
