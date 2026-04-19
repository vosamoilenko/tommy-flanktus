# deploy.ps1 — Compile, upload, health-check, and monitor FLANKTUS
# Usage: double-click deploy.bat
#   or:  powershell -ExecutionPolicy Bypass -File deploy.ps1
#   or:  powershell -ExecutionPolicy Bypass -File deploy.ps1 COM3

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
    Write-Host "ERROR: No Arduino Uno found. Check USB connection." -ForegroundColor Red
    Write-Host ""
    Write-Host "Available boards:"
    & arduino-cli board list
    Read-Host "Press Enter to exit"
    exit 1
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Split-Path -Parent $scriptDir
$sketchDir = Join-Path $projectDir "flanktus_pump"
$logFile = Join-Path $projectDir "logs\sensor_log.txt"

# ── Compile ──
Write-Host ""
Write-Host "Compiling..."
& arduino-cli compile --fqbn arduino:avr:uno $sketchDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "Compile failed." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# ── Upload ──
Write-Host ""
Write-Host "Uploading to $comPort..."
& arduino-cli upload -p $comPort --fqbn arduino:avr:uno $sketchDir
if ($LASTEXITCODE -ne 0) {
    Write-Host "Upload failed." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host "Upload OK." -ForegroundColor Green

# ── Wait for Arduino reboot ──
Write-Host ""
Write-Host "Waiting for Arduino to reboot..."
Start-Sleep -Seconds 3

# ── Open serial port ──
try {
    $port = New-Object System.IO.Ports.SerialPort $comPort, 9600, "None", 8, "One"
    $port.ReadTimeout = 1000
    $port.Open()
} catch {
    Write-Host "ERROR: Cannot open $comPort — $($_.Exception.Message)" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# ── Health check (10 seconds) ──
Write-Host ""
Write-Host "Running health check (10s)..." -ForegroundColor Yellow
Write-Host ("=" * 40)

$healthLines = @()
$endTime = (Get-Date).AddSeconds(10)

while ((Get-Date) -lt $endTime) {
    try {
        $line = $port.ReadLine()
        Write-Host "  $line"
        $healthLines += $line
    } catch [System.TimeoutException] {
        # no data yet, keep waiting
    }
}

Write-Host ("=" * 40)
Write-Host ""

$errors = 0

# Check boot message
if ($healthLines -match "FLANKTUS") {
    Write-Host "OK    Arduino is running" -ForegroundColor Green
} else {
    Write-Host "FAIL  No boot message received - check USB connection" -ForegroundColor Red
    $errors++
}

# Check debug output
$dbgLines = $healthLines | Where-Object { $_ -match "\[DBG" }

if ($dbgLines) {
    # Water sensor
    if ($dbgLines -match "water=-99\.9") {
        Write-Host "WARN  Water sensor disconnected (reads -99.9C)" -ForegroundColor Yellow
    } else {
        Write-Host "OK    Water sensor connected" -ForegroundColor Green
    }

    # Air sensor
    if ($dbgLines -match "air=-99\.9") {
        Write-Host "WARN  Air sensor disconnected (reads -99.9C)" -ForegroundColor Yellow
    } else {
        Write-Host "OK    Air sensor connected" -ForegroundColor Green
    }

    # Auto mode
    if ($dbgLines -match "auto=ON") {
        Write-Host "OK    Auto mode active - pump is cycling" -ForegroundColor Green
    } else {
        Write-Host "WARN  Auto mode is OFF" -ForegroundColor Yellow
    }
} else {
    Write-Host "FAIL  No debug output - something is wrong" -ForegroundColor Red
    $errors++
}

Write-Host ""
if ($errors -gt 0) {
    Write-Host "Health check found $errors error(s). Check wiring and connections." -ForegroundColor Red
    Write-Host "Continuing to serial monitor anyway..."
}

# ── Stream serial output ──
Write-Host ""
Write-Host "Streaming serial output... (Ctrl+C to stop)" -ForegroundColor Cyan
Write-Host "Saving to $logFile"
Write-Host ("=" * 40)

# Ensure logs directory exists
$logDir = Split-Path -Parent $logFile
if (-not (Test-Path $logDir)) { New-Item -ItemType Directory -Path $logDir | Out-Null }

try {
    while ($true) {
        try {
            $line = $port.ReadLine()
            Write-Host $line
            Add-Content -Path $logFile -Value $line
        } catch [System.TimeoutException] {
            # no data, keep waiting
        }
    }
} finally {
    $port.Close()
    Write-Host ""
    Write-Host "Serial port closed." -ForegroundColor Yellow
}
