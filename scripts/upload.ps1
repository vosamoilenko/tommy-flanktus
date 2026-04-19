# upload.ps1 - Compile and upload FLANKTUS to Arduino
# Usage: double-click upload.bat
#   or:  powershell -ExecutionPolicy Bypass -File upload.ps1
#   or:  powershell -ExecutionPolicy Bypass -File upload.ps1 COM3

$ErrorActionPreference = "Stop"

# Ensure arduino-cli is available — check common install locations if not in PATH
if (-not (Get-Command "arduino-cli" -ErrorAction SilentlyContinue)) {
    $candidates = @(
        "$env:USERPROFILE\arduino-cli.exe",
        "$env:LOCALAPPDATA\Arduino15\arduino-cli.exe",
        "$env:LOCALAPPDATA\Programs\arduino-cli\arduino-cli.exe",
        "C:\Program Files\Arduino CLI\arduino-cli.exe",
        "C:\arduino-cli\arduino-cli.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) {
            $env:PATH = (Split-Path $c) + ";" + $env:PATH
            Write-Host "Found arduino-cli at $c" -ForegroundColor Cyan
            break
        }
    }
    if (-not (Get-Command "arduino-cli" -ErrorAction SilentlyContinue)) {
        Write-Host "ERROR: arduino-cli not found." -ForegroundColor Red
        Write-Host ""
        Write-Host "Install it from: https://arduino.github.io/arduino-cli/installation/"
        Write-Host "Or add its folder to your PATH environment variable."
        Read-Host "Press Enter to exit"
        exit 1
    }
}

$comPort = $null
if ($args.Count -ge 1) {
    $comPort = $args[0]
    Write-Host "Using port: $comPort (manual)" -ForegroundColor Cyan
}

# Auto-detect via arduino-cli
if (-not $comPort) {
    $boardLine = & arduino-cli board list 2>&1 | Select-String "arduino:avr:uno"
    if ($boardLine) {
        $comPort = ($boardLine -split '\s+')[0]
        Write-Host "Found Arduino on $comPort" -ForegroundColor Green
    }
}

if (-not $comPort) {
    Write-Host "No Arduino found. Check USB connection." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Split-Path -Parent $scriptDir
$sketchDir = Join-Path $projectDir "flanktus_pump"

Write-Host "Compiling..."
& arduino-cli compile --fqbn arduino:avr:uno $sketchDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "Compile failed." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host "Uploading to $comPort..."
& arduino-cli upload -p $comPort --fqbn arduino:avr:uno $sketchDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "Upload failed." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Write-Host ""
Write-Host "Done! Sketch uploaded to $comPort" -ForegroundColor Green
Read-Host "Press Enter to exit"
