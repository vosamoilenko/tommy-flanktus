# clear_eeprom.ps1 - Clear the EEPROM ring buffer
# Usage: double-click clear_eeprom.bat
#   or:  powershell -ExecutionPolicy Bypass -File clear_eeprom.ps1
#   or:  powershell -ExecutionPolicy Bypass -File clear_eeprom.ps1 COM3

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

$serial = New-Object System.IO.Ports.SerialPort $comPort, 9600
$serial.DtrEnable = $true

try {
    $serial.Open()
} catch {
    Write-Host "Cannot open $comPort. Is another program using it?" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Wait for Arduino to boot after serial open
Start-Sleep -Milliseconds 3000
$serial.DiscardInBuffer()

$serial.WriteLine("c")
Start-Sleep -Milliseconds 500

# Read confirmation
try {
    $serial.ReadTimeout = 2000
    $response = $serial.ReadLine().Trim()
    Write-Host $response
} catch {
    Write-Host "Sent clear command."
}

$serial.Close()
Write-Host "EEPROM log cleared." -ForegroundColor Green
Read-Host "Press Enter to exit"
