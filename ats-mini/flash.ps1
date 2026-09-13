param (
    [string]$Port = ""
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   ATS-Mini Hybrid Firmware Flasher (SI4732 + Web Radio)   " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# If no port specified, attempt auto-detection via arduino-cli
if ([string]::IsNullOrWhiteSpace($Port)) {
    Write-Host "Detecting connected serial devices..." -ForegroundColor Yellow
    $boardOutput = arduino-cli board list
    Write-Host $boardOutput

    # Check for available COM ports
    $comMatch = [regex]::Matches($boardOutput, 'COM\d+')
    if ($comMatch.Count -gt 0) {
        $Port = $comMatch[0].Value
        Write-Host "Auto-selected port: $Port" -ForegroundColor Green
    } else {
        Write-Host "No COM port detected automatically." -ForegroundColor Red
        $Port = Read-Host "Please enter your ESP32-S3 COM port (e.g. COM3 or COM4)"
    }
}

if ([string]::IsNullOrWhiteSpace($Port)) {
    Write-Host "Error: No COM port provided. Aborting." -ForegroundColor Red
    exit 1
}

Write-Host "`nFlashing ATS-Mini Hybrid Firmware to $Port..." -ForegroundColor Cyan

$sketchDir = "$PSScriptRoot"
$fqbn = "esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=8M,PSRAM=opi,CPUFreq=80,USBMode=hwcdc,FlashMode=qio,PartitionScheme=custom,DebugLevel=none"

$cmd = "arduino-cli upload -p $Port --fqbn $fqbn `"$sketchDir`""
Write-Host "Running: $cmd" -ForegroundColor Gray

Invoke-Expression $cmd

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n[SUCCESS] Firmware uploaded successfully to $Port!" -ForegroundColor Green
    Write-Host "Your ATS-Mini is now running the Hybrid SI4732 + Internet Radio firmware." -ForegroundColor Green
} else {
    Write-Host "`n[ERROR] Upload failed. Please check your cable, port, and bootloader status." -ForegroundColor Red
}
