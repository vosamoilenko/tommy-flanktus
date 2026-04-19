# upload.ps1 - Compile and upload FLANKTUS to Arduino
# Usage: double-click upload.bat
#   or:  powershell -ExecutionPolicy Bypass -File upload.ps1
#   or:  powershell -ExecutionPolicy Bypass -File upload.ps1 COM3

$ErrorActionPreference = "Stop"

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
