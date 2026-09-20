# Install the bundled EvoFlow Python package through one .pth file.
# This script is intentionally ASCII-only for Windows PowerShell 5.1.
param(
    [Parameter(Mandatory=$true)][string]$PythonExe,
    [string]$PackageRoot = "",
    [string]$TargetDirectory = "",
    [switch]$Remove
)

$ErrorActionPreference = "Stop"
$releaseRoot = $PSScriptRoot
if (-not (Test-Path -LiteralPath $PythonExe)) {
    throw "Python executable not found: $PythonExe"
}

$metadataJson = & $PythonExe -c "import importlib.machinery,json,site,struct,sys; print(json.dumps({'implementation':sys.implementation.name,'major':sys.version_info[0],'minor':sys.version_info[1],'bits':8*struct.calcsize('P'),'ext_suffixes':importlib.machinery.EXTENSION_SUFFIXES,'user_site':site.getusersitepackages()}))"
if ($LASTEXITCODE -ne 0) {
    throw "Failed to query Python installation metadata"
}
$metadata = $metadataJson | ConvertFrom-Json
if ([string]$metadata.implementation -ne "cpython") {
    throw "Only CPython is supported by the bundled native module"
}
if ([int]$metadata.bits -ne 64) {
    throw "Only 64-bit CPython is supported by the bundled native module"
}

if (-not $PackageRoot) {
    $PackageRoot = Join-Path $releaseRoot "python"
}
if (-not $TargetDirectory) {
    $TargetDirectory = [string]$metadata.user_site
}
if (-not $TargetDirectory) {
    throw "Python did not report a user site-packages directory"
}
New-Item -ItemType Directory -Force -Path $TargetDirectory | Out-Null
$TargetDirectory = (Resolve-Path -LiteralPath $TargetDirectory).Path
$pthPath = Join-Path $TargetDirectory "evoflow-release.pth"

if ($Remove) {
    if (Test-Path -LiteralPath $pthPath) {
        Remove-Item -LiteralPath $pthPath -Force
        Write-Host "Removed: $pthPath"
    }
    else {
        Write-Host "Already absent: $pthPath"
    }
    exit 0
}

if (-not (Test-Path -LiteralPath $PackageRoot)) {
    throw "EvoFlow Python package root not found: $PackageRoot"
}
$PackageRoot = (Resolve-Path -LiteralPath $PackageRoot).Path
$packageDir = Join-Path $PackageRoot "evoflow"
if (-not (Test-Path -LiteralPath (Join-Path $packageDir "__init__.py"))) {
    throw "EvoFlow package is missing under: $PackageRoot"
}

$extensionNames = @($metadata.ext_suffixes | ForEach-Object { "_core" + [string]$_ })
$extensionName = @($extensionNames | Where-Object { Test-Path -LiteralPath (Join-Path $packageDir $_) } | Select-Object -First 1)
if ($extensionName.Count -eq 0) {
    $available = @(Get-ChildItem -LiteralPath $packageDir -Filter "_core*.pyd" -File -ErrorAction SilentlyContinue | ForEach-Object { $_.Name })
    $availableText = if ($available.Count -gt 0) { $available -join ", " } else { "none" }
    throw "No native module for CPython $($metadata.major).$($metadata.minor) 64-bit. Accepted names: $($extensionNames -join ', '); bundled: $availableText"
}

[System.IO.File]::WriteAllText(
    $pthPath,
    $PackageRoot + [Environment]::NewLine,
    [System.Text.Encoding]::Default
)

$oldPythonPath = $env:PYTHONPATH
try {
    $env:PYTHONPATH = $null
    $verified = & $PythonExe -c "import site,sys; site.addsitedir(sys.argv[1]); import evoflow; print(evoflow.__version__)" $TargetDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "The .pth file was written but EvoFlow import verification failed"
    }
}
finally {
    $env:PYTHONPATH = $oldPythonPath
}

Write-Host "Installed: $pthPath"
Write-Host "Package root: $PackageRoot"
Write-Host "Verified EvoFlow version: $verified"
