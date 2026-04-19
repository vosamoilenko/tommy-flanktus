# read_logs.ps1 - Stream live debug logs from Arduino
# Usage: double-click read_logs.bat
#   or:  powershell -ExecutionPolicy Bypass -File read_logs.ps1
#   or:  powershell -ExecutionPolicy Bypass -File read_logs.ps1 30
#   or:  powershell -ExecutionPolicy Bypass -File read_logs.ps1 30 COM3
#
# First argument:  duration in seconds (omit to stream until Ctrl+C)
# Second argument: COM port (omit to auto-detect)

$ErrorActionPreference = "Stop"

$duration = $null
$comPort = $null

if ($args.Count -ge 1) { $duration = [int]$args[0] }
if ($args.Count -ge 2) { $comPort = $args[1] }

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
$logFile = Join-Path $projectDir "logs\sensor_log.txt"

Write-Host "Reading from $comPort (9600 baud)..."
Write-Host "Saving to $logFile"

$serial = New-Object System.IO.Ports.SerialPort $comPort, 9600
$serial.ReadTimeout = 2000
$serial.DtrEnable = $true

try {
    $serial.Open()
} catch {
    Write-Host "Cannot open $comPort. Is another program using it?" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

Start-Sleep -Milliseconds 2000
$serial.DiscardInBuffer()

$lines = @()
$deadline = $null
if ($duration) {
    Write-Host "Capturing for ${duration}s..."
    $deadline = (Get-Date).AddSeconds($duration)
} else {
    Write-Host "Streaming... press Ctrl+C to stop."
}

try {
    while ($true) {
        if ($deadline -and (Get-Date) -ge $deadline) { break }
        try {
            $line = $serial.ReadLine().Trim()
            if ($line -eq "") { continue }
            Write-Host $line
            $lines += $line
        } catch [System.TimeoutException] {
            continue
        }
    }
} finally {
    $serial.Close()
    if ($lines.Count -gt 0) {
        $lines | Out-File -FilePath $logFile -Encoding UTF8
        Write-Host ""
        Write-Host "$($lines.Count) lines saved to $logFile" -ForegroundColor Green
    }
}
