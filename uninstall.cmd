@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\uninstall.ps1" %*
if errorlevel 1 (
    echo.
    echo Removal failed. Read the message above.
)
echo.
pause
