# Builds gamemodes\rp.pwn -> gamemodes\rp.amx on Windows.
# (Kept ASCII-only on purpose: Windows PowerShell misreads UTF-8 scripts.)
#
# Usage (PowerShell):
#   powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Server C:\omp-server
#
# -Server is the open.mp server folder (it contains qawno\pawncc.exe).

param(
    [string]$Server = (Join-Path $PSScriptRoot "..\..\server")
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")
$Pawncc = Join-Path $Server "qawno\pawncc.exe"
$Include = Join-Path $Server "qawno\include"
$Src = Join-Path $Root "build\src"

if (-not (Test-Path $Pawncc)) {
    Write-Host "pawncc.exe not found: $Pawncc"
    Write-Host "Use: scripts\build.ps1 -Server C:\path\to\server"
    exit 1
}

# PowerShell 7 (.NET Core) needs this for code page 1251.
try {
    [System.Text.Encoding]::RegisterProvider([System.Text.CodePagesEncodingProvider]::Instance)
} catch {}

if (Test-Path $Src) { Remove-Item -Recurse -Force $Src }
New-Item -ItemType Directory -Force -Path $Src | Out-Null

# UTF-8 -> Windows-1251 (Cyrillic for the SA-MP client)
$utf8 = New-Object System.Text.UTF8Encoding($false)
$cp1251 = [System.Text.Encoding]::GetEncoding(1251)
$Gamemodes = Join-Path $Root "gamemodes"

Get-ChildItem -Path $Gamemodes -Recurse -Include *.pwn, *.inc | ForEach-Object {
    $rel = $_.FullName.Substring($Gamemodes.ToString().Length + 1)
    $dest = Join-Path $Src $rel
    New-Item -ItemType Directory -Force -Path (Split-Path $dest) | Out-Null
    $text = [System.IO.File]::ReadAllText($_.FullName, $utf8)
    [System.IO.File]::WriteAllText($dest, $text, $cp1251)
}

& $Pawncc (Join-Path $Src "rp.pwn") "-i$Include" "-i$Src" "-o$(Join-Path $Gamemodes 'rp.amx')" -d3 "-;+" -O1
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Done: gamemodes\rp.amx"
