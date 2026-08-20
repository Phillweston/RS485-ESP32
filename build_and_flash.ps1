param(
    [string]$Port = 'COM37',
    [switch]$CompileOnly
)

$ErrorActionPreference = 'Stop'

$config = Join-Path $PSScriptRoot 'fsc2a.yaml'
$secrets = Join-Path $PSScriptRoot 'secrets.yaml'

if (-not (Get-Command uvx -ErrorAction SilentlyContinue)) {
    throw 'uvx was not found. Install uv from https://docs.astral.sh/uv/ first.'
}

if (-not (Test-Path -LiteralPath $config)) {
    throw "Missing ESPHome configuration: $config"
}

if (-not (Test-Path -LiteralPath $secrets)) {
    throw "Missing $secrets. Create it from secrets.yaml.example."
}

Write-Host "[FSC-2A] ESPHome config: $config"
& uvx --python 3.13 esphome config $config
if ($LASTEXITCODE -ne 0) {
    throw 'ESPHome configuration validation failed.'
}

if ($CompileOnly) {
    Write-Host '[FSC-2A] Compile only mode.'
    & uvx --python 3.13 esphome compile $config
} else {
    if ($Port -notin [System.IO.Ports.SerialPort]::GetPortNames()) {
        throw "Serial port $Port was not found. Use -Port COMx to select the StickS3 port."
    }

    Write-Host "[FSC-2A] Flashing device on $Port."
    & uvx --python 3.13 esphome run $config --device $Port
}

if ($LASTEXITCODE -ne 0) {
    throw "ESPHome exited with code $LASTEXITCODE."
}
