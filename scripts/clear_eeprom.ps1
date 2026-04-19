# clear_eeprom.ps1 - Clear the EEPROM ring buffer
# Usage: double-click clear_eeprom.bat
#   or:  powershell -ExecutionPolicy Bypass -File clear_eeprom.ps1
#   or:  powershell -ExecutionPolicy Bypass -File clear_eeprom.ps1 COM3

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
