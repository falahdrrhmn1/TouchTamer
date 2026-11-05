@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\install.ps1" %*
if errorlevel 1 (
    echo.
    echo Installation failed. Read the message above.
)
echo.
pause
