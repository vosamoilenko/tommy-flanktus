# clean_csv.ps1 — Remove rows with -99 (disconnected sensor) from exported CSV
# Usage: powershell -ExecutionPolicy Bypass -File clean_csv.ps1 input.csv [output.csv]
#   If no output file given, prints to console.

if ($args.Count -lt 1) {
    Write-Host "Usage: clean_csv.ps1 input.csv [output.csv]" -ForegroundColor Yellow
    exit 1
}

$inputFile = $args[0]
$outputFile = $null
if ($args.Count -ge 2) { $outputFile = $args[1] }

if (-not (Test-Path $inputFile)) {
    Write-Host "File not found: $inputFile" -ForegroundColor Red
    exit 1
}

$allLines = Get-Content $inputFile
$header = $allLines[0]
$dataLines = $allLines | Select-Object -Skip 1
$cleanLines = $dataLines | Where-Object { $_ -notmatch '-99' }

if ($outputFile) {
    $header | Out-File -FilePath $outputFile -Encoding UTF8
    $cleanLines | Out-File -FilePath $outputFile -Encoding UTF8 -Append
    $total = $dataLines.Count
    $clean = $cleanLines.Count
    Write-Host "Cleaned: $clean/$total rows kept -> $outputFile" -ForegroundColor Green
} else {
    Write-Host $header
    $cleanLines | ForEach-Object { Write-Host $_ }
}
