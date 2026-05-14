# setup.ps1 - First-time Windows setup for FLANKTUS v6.0
# Installs Arduino core and all required libraries.
# Usage: powershell -ExecutionPolicy Bypass -File setup.ps1

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "FLANKTUS v6.0 - Windows Setup" -ForegroundColor Green
Write-Host "==============================" -ForegroundColor Green
Write-Host ""

# ── Find arduino-cli ──
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
        Write-Host "Download it from: https://arduino.github.io/arduino-cli/installation/" -ForegroundColor Yellow
        Write-Host "Extract arduino-cli.exe to one of these locations:"
        Write-Host "  - $env:USERPROFILE\arduino-cli.exe"
        Write-Host "  - C:\arduino-cli\arduino-cli.exe"
        Write-Host "Or add its folder to your PATH."
        Write-Host ""
        Read-Host "Press Enter to exit"
        exit 1
    }
}

Write-Host "arduino-cli version:" -ForegroundColor Cyan
& arduino-cli version
Write-Host ""

# ── Update core index ──
Write-Host "Updating board index..." -ForegroundColor Yellow
& arduino-cli core update-index
if ($LASTEXITCODE -ne 0) {
    Write-Host "WARNING: Could not update board index." -ForegroundColor Yellow
}
Write-Host ""

# ── Install Arduino AVR core ──
Write-Host "Installing Arduino AVR core..." -ForegroundColor Yellow
& arduino-cli core install arduino:avr
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Failed to install Arduino AVR core." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host "Arduino AVR core: OK" -ForegroundColor Green
Write-Host ""

# ── Install libraries ──
$libs = @(
    "OneWire",
    "DallasTemperature",
    "SD",
    "LiquidCrystal I2C"
)

foreach ($lib in $libs) {
    Write-Host "Installing library: $lib ..." -ForegroundColor Yellow
    & arduino-cli lib install "$lib"
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to install $lib" -ForegroundColor Red
        Read-Host "Press Enter to exit"
        exit 1
    }
    Write-Host "  $lib : OK" -ForegroundColor Green
}

Write-Host ""

# ── Verify compilation ──
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Split-Path -Parent $scriptDir
$sketchDir = Join-Path $projectDir "flanktus_pump"

Write-Host "Test compiling sketch..." -ForegroundColor Yellow
& arduino-cli compile --fqbn arduino:avr:uno $sketchDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "Compile FAILED." -ForegroundColor Red
    Write-Host "Check the error output above." -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host ""
Write-Host "Compile: OK" -ForegroundColor Green

# ── Check for connected Arduino ──
Write-Host ""
Write-Host "Checking for connected Arduino..." -ForegroundColor Yellow
$boardLine = & arduino-cli board list 2>&1 | Select-String "arduino:avr:uno"
if ($boardLine) {
    $comPort = ($boardLine -split '\s+')[0]
    Write-Host "Arduino Uno found on $comPort" -ForegroundColor Green
} else {
    Write-Host "No Arduino Uno detected (plug it in before deploying)." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Available boards:"
    & arduino-cli board list
}

Write-Host ""
Write-Host "==============================" -ForegroundColor Green
Write-Host "Setup complete!" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Connect all hardware (see docs/flanktus-wiring-guide.html)"
Write-Host "  2. Plug Arduino into USB"
Write-Host "  3. Run: deploy.bat" -ForegroundColor Yellow
Write-Host ""
Read-Host "Press Enter to exit"
