# Removes TouchTamer for the current user. Only TouchTamer's own files are deleted.
param(
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA 'Programs\TouchTamer'),
    [string]$StartupDir = [Environment]::GetFolderPath('Startup')
)
$ErrorActionPreference = 'Stop'

Write-Host ''
Write-Host 'TouchTamer uninstaller'
Write-Host ''

$exe = Join-Path $InstallDir 'TouchTamer.exe'
$shortcut = Join-Path $StartupDir 'TouchTamer.lnk'

if (Test-Path $exe) {
    $running = @(Get-Process TouchTamer -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe })
    if ($running.Count -gt 0) {
        Start-Process -FilePath $exe -ArgumentList '--exit' -Wait
        for ($i = 0; $i -lt 30 -and @(Get-Process TouchTamer -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe }).Count -gt 0; $i++) {
            Start-Sleep -Milliseconds 100
        }
        Get-Process TouchTamer -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe } | Stop-Process -Force
        Start-Sleep -Milliseconds 300
    }
    Write-Host '  Stopped TouchTamer.'
}

if (Test-Path $shortcut) {
    Remove-Item -LiteralPath $shortcut -Force
    Write-Host '  Removed the startup shortcut.'
}

foreach ($name in 'TouchTamer.exe', 'TouchTamer.ini', 'TouchTamer.log') {
    $file = Join-Path $InstallDir $name
    if (Test-Path $file) { Remove-Item -LiteralPath $file -Force }
}
if ((Test-Path $InstallDir) -and @(Get-ChildItem -LiteralPath $InstallDir -Force).Count -eq 0) {
    Remove-Item -LiteralPath $InstallDir -Force
}
Write-Host '  Removed the program files.'
Write-Host ''
Write-Host 'TouchTamer is gone. Your brightness and volume stay at their current levels.'
