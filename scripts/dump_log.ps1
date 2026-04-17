# dump_log.ps1 — Export FLANKTUS EEPROM log to CSV file
# Usage: double-click dump_log.bat
#   or:  powershell -ExecutionPolicy Bypass -File dump_log.ps1
#   or:  powershell -ExecutionPolicy Bypass -File dump_log.ps1 COM3
#
# How it finds the Arduino:
#   1. Queries Windows for USB serial devices whose hardware ID
#      contains VID_2341 (Arduino) or VID_1A86 (CH340 clone).
#   2. Picks the first matching COM port.
#   3. If nothing matches, lists all available COM ports and asks
#      the user to type the right one.
#   No blind probing — the port is only opened once, to dump data.

$ErrorActionPreference = "Stop"

# --- Manual override from command line ---
$comPort = $null
if ($args.Count -ge 1) {
    $comPort = $args[0]
    Write-Host "Using port: $comPort (manual)" -ForegroundColor Cyan
}

# --- Auto-detect Arduino by USB vendor ID ---
if (-not $comPort) {
    # Known Arduino USB vendor IDs:
    #   2341 = Arduino official
    #   1A86 = CH340 (common clone chip)
    #   0403 = FTDI
    #   2A03 = Arduino.org boards
    $usbDevices = Get-CimInstance Win32_PnPEntity -Filter "Name LIKE '%COM%'" |
        Where-Object {
            $_.PNPDeviceID -match "VID_2341" -or   # Arduino
            $_.PNPDeviceID -match "VID_1A86" -or   # CH340
            $_.PNPDeviceID -match "VID_0403" -or   # FTDI
            $_.PNPDeviceID -match "VID_2A03"        # Arduino.org
        }

    foreach ($dev in $usbDevices) {
        if ($dev.Name -match "\((COM\d+)\)") {
            $comPort = $Matches[1]
            Write-Host "Found Arduino: $($dev.Name)" -ForegroundColor Green
            break
        }
    }
}

# --- Fallback: ask the user ---
if (-not $comPort) {
    $allPorts = [System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object
    Write-Host ""
    Write-Host "Could not auto-detect Arduino." -ForegroundColor Yellow
    if ($allPorts.Count -eq 0) {
        Write-Host "No COM ports found. Is the USB cable plugged in?" -ForegroundColor Red
        Read-Host "Press Enter to exit"
        exit 1
    }
    Write-Host "Available COM ports: $($allPorts -join ', ')"
    Write-Host ""
    $comPort = Read-Host "Type your Arduino port (e.g. COM3)"
    if (-not $comPort) { exit 1 }
}

# --- Open serial and dump ---
Write-Host "Opening $comPort at 9600 baud..."

$serial = New-Object System.IO.Ports.SerialPort $comPort, 9600
$serial.ReadTimeout = 5000
$serial.DtrEnable = $true

try {
    $serial.Open()
} catch {
    Write-Host "Cannot open $comPort. Is another program using it?" -ForegroundColor Red
    Write-Host "(Close Arduino IDE Serial Monitor if open)" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

# Arduino resets when serial opens — wait for boot message
Write-Host "Waiting for Arduino to boot..."
Start-Sleep -Milliseconds 3000
$serial.DiscardInBuffer()

# Send dump command
$serial.WriteLine("d")
Write-Host "Requesting EEPROM dump..."

# --- Read lines until summary line ---
$lines = @()
$deadline = (Get-Date).AddSeconds(30)

while ((Get-Date) -lt $deadline) {
    try {
        $line = $serial.ReadLine().Trim()
        if ($line -eq "") { continue }
        $lines += $line
        # Stop when we see "--- 42/170 entries ---"
        if ($line -match "^---.*entries ---$") { break }
    } catch {
        break
    }
}

$serial.Close()

if ($lines.Count -eq 0) {
    Write-Host "No data received. Is the Arduino running FLANKTUS?" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# --- Save to file ---
$timestamp = Get-Date -Format "yyyy-MM-dd_HHmm"
$filename = "flanktus_dump_$timestamp.csv"

# CSV lines only (skip the "--- X/170 entries ---" summary)
$csvLines = $lines | Where-Object { $_ -notmatch "^---" }
$csvLines | Out-File -FilePath $filename -Encoding UTF8

# --- Print results ---
Write-Host ""
Write-Host "=== DUMP ===" -ForegroundColor Cyan
foreach ($l in $lines) { Write-Host $l }
Write-Host ""
Write-Host "Saved to: $filename" -ForegroundColor Green
$dataCount = $csvLines.Count - 1  # minus header row
Write-Host "$dataCount data entries" -ForegroundColor Green
Write-Host ""
Write-Host "Send this file: $filename" -ForegroundColor Yellow
Write-Host ""
Read-Host "Press Enter to exit"
