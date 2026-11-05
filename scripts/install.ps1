# Installs TouchTamer for the current user. No administrator rights are needed.
param(
    [string]$InstallDir = (Join-Path $env:LOCALAPPDATA 'Programs\TouchTamer'),
    [string]$StartupDir = [Environment]::GetFolderPath('Startup'),
    [switch]$NoStart
)
$ErrorActionPreference = 'Stop'

function Write-Step($text) { Write-Host "  $text" }

function Get-TouchpadInfo {
    # SPI_GETTOUCHPADPARAMETERS exists on Windows 11 24H2 and later. Older versions return nothing.
    try {
        Add-Type -ErrorAction Stop @'
using System; using System.Runtime.InteropServices;
public static class TouchTamerSpi {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SystemParametersInfoW(uint action, uint size, uint[] data, uint flags);
}
'@
        $data = New-Object 'uint32[]' 11
        $data[0] = 1
        if ([TouchTamerSpi]::SystemParametersInfoW(0xAE, 44, $data, 0)) {
            return [pscustomobject]@{ MaxContacts = [int]$data[1]; Present = (($data[3] -band 1) -eq 1) }
        }
    } catch { }
    return $null
}

function Stop-Running($exePath) {
    $running = @(Get-Process TouchTamer -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exePath })
    if ($running.Count -eq 0) { return }
    Start-Process -FilePath $exePath -ArgumentList '--exit' -Wait
    for ($i = 0; $i -lt 30; $i++) {
        $running = @(Get-Process TouchTamer -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exePath })
        if ($running.Count -eq 0) { return }
        Start-Sleep -Milliseconds 100
    }
    $running | Stop-Process -Force
}

Write-Host ''
Write-Host 'TouchTamer installer'
Write-Host ''

$repo = Split-Path -Parent $PSScriptRoot
$sourceExe = Join-Path $repo 'bin\TouchTamer.exe'
$sourceIni = Join-Path $repo 'bin\TouchTamer.ini'
if (-not (Test-Path $sourceExe) -or -not (Test-Path $sourceIni)) {
    throw "bin\TouchTamer.exe or bin\TouchTamer.ini is missing. Extract the whole ZIP file first, then run this again."
}
if (-not [Environment]::Is64BitOperatingSystem) { throw 'TouchTamer needs 64-bit Windows.' }
if ([Environment]::OSVersion.Version.Major -lt 10) { throw 'TouchTamer needs Windows 10 or Windows 11.' }

$pad = Get-TouchpadInfo
if ($null -eq $pad) {
    Write-Step 'Touchpad check: not available on this Windows version. See "Check your touchpad" in the README.'
} elseif (-not $pad.Present) {
    Write-Step 'WARNING: Windows reports no Precision Touchpad. Gestures will not work; Alt+arrow hotkeys still will.'
} else {
    Write-Step "Touchpad check: Precision Touchpad found, up to $($pad.MaxContacts) fingers."
    if ($pad.MaxContacts -lt 5) {
        Write-Step 'WARNING: this touchpad reads fewer than 5 fingers, so the five-finger window move will not work.'
    }
}

$targetExe = Join-Path $InstallDir 'TouchTamer.exe'
$targetIni = Join-Path $InstallDir 'TouchTamer.ini'
$shortcut = Join-Path $StartupDir 'TouchTamer.lnk'

Stop-Running $targetExe

New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
Copy-Item -LiteralPath $sourceExe -Destination $targetExe -Force
Unblock-File -LiteralPath $targetExe
if (Test-Path $targetIni) {
    Write-Step 'Settings: kept your existing TouchTamer.ini.'
} else {
    Copy-Item -LiteralPath $sourceIni -Destination $targetIni
    Write-Step 'Settings: created TouchTamer.ini.'
}
Write-Step "Installed to: $InstallDir"

New-Item -ItemType Directory -Force -Path $StartupDir | Out-Null
$shell = New-Object -ComObject WScript.Shell
$link = $shell.CreateShortcut($shortcut)
$link.TargetPath = $targetExe
$link.WorkingDirectory = $InstallDir
$link.IconLocation = "$targetExe,0"
$link.Description = 'TouchTamer touchpad gestures'
$link.Save()
Write-Step 'Startup: TouchTamer will start when you sign in to Windows.'

if (-not $NoStart) {
    Start-Process -FilePath $targetExe -WorkingDirectory $InstallDir
    Write-Step 'TouchTamer is running now. It has no window; it works in the background.'
}

Write-Host ''
Write-Host 'Try it:'
Write-Host '  Brightness   hold Ctrl+Shift, swipe up or down with 2 fingers on the LEFT half of the touchpad'
Write-Host '  Volume       hold Ctrl+Shift, swipe up or down with 2 fingers on the RIGHT half of the touchpad'
Write-Host '  Move window  put the cursor over a window, then swipe with 5 fingers'
Write-Host '  Window size  Alt+Up and Alt+Down'
Write-Host '  Window width Alt+Right and Alt+Left'
Write-Host ''
Write-Host 'To remove TouchTamer, run uninstall.cmd.'
