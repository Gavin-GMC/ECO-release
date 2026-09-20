# EvoFlow Python binding build helper for Windows x64.
# This script is intentionally ASCII-only for Windows PowerShell 5.1.
# It does not use CMake and does not change the existing /MT exe build.
param(
    [Parameter(Mandatory=$true)][string]$PythonExe,
    [switch]$Parallel,
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"
$scriptRoot = $PSScriptRoot
$branch = [System.IO.Path]::GetFullPath((Join-Path $scriptRoot "..\.."))
$vcvars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

if ((-not (Test-Path -LiteralPath (Join-Path $branch "src") -PathType Container)) -or
    (-not (Test-Path -LiteralPath (Join-Path $branch "python") -PathType Container))) {
    throw "Cannot locate the EvoFlow release root from: $scriptRoot"
}

if (-not (Test-Path -LiteralPath $PythonExe)) {
    throw "Python executable not found: $PythonExe"
}
if (-not (Test-Path -LiteralPath $vcvars)) {
    throw "Visual Studio vcvars64.bat not found: $vcvars"
}

$metadataJson = & $PythonExe -c "import json,struct,sys,sysconfig; print(json.dumps({'implementation':sys.implementation.name,'major':sys.version_info[0],'minor':sys.version_info[1],'bits':8*struct.calcsize('P'),'include':sysconfig.get_path('include'),'base_prefix':sys.base_prefix}))"
if ($LASTEXITCODE -ne 0) {
    throw "Failed to query Python build metadata"
}
$metadata = $metadataJson | ConvertFrom-Json
if ([string]$metadata.implementation -ne "cpython") {
    throw "Only CPython is supported by this build"
}
if ([int]$metadata.major -ne 3) {
    throw "Only Python 3 is supported by this probe"
}
if ([int]$metadata.bits -ne 64) {
    throw "Only 64-bit Python is supported by this Windows probe"
}

$pythonTag = "cp$($metadata.major)$($metadata.minor)-win_amd64"
$pythonInclude = [string]$metadata.include
$pythonLib = Join-Path ([string]$metadata.base_prefix) "libs\python$($metadata.major)$($metadata.minor).lib"
$pybindInclude = Join-Path $branch "thirdparty\pybind11\include"
if (-not (Test-Path -LiteralPath (Join-Path $pythonInclude "Python.h"))) {
    throw "Python.h not found under: $pythonInclude"
}
if (-not (Test-Path -LiteralPath $pythonLib)) {
    throw "Python import library not found: $pythonLib"
}
if (-not (Test-Path -LiteralPath (Join-Path $pybindInclude "pybind11\pybind11.h"))) {
    throw "Vendored pybind11 headers not found: $pybindInclude"
}

$mode = if ($Parallel) { "parallel" } else { "serial" }
if ($Parallel) {
    $objRoot = Join-Path $branch "obj-python-parallel"
}
else {
    $objRoot = Join-Path $branch "obj-python"
}
$objDir = Join-Path $objRoot "obj"
$tempDir = Join-Path $objRoot "tmp"
if ($Parallel) {
    $packageRoot = Join-Path $objRoot "python"
}
else {
    $packageRoot = Join-Path $branch "python"
}
$packageDir = Join-Path $packageRoot "evoflow"
New-Item -ItemType Directory -Force -Path $objDir, $tempDir, $packageDir | Out-Null
if ($Parallel) {
    Copy-Item -Path (Join-Path $branch "python\evoflow\*.py") -Destination $packageDir -Force
    Copy-Item -Path (Join-Path $branch "python\evoflow\*.pyi") -Destination $packageDir -Force
    Copy-Item -LiteralPath (Join-Path $branch "python\evoflow\py.typed") -Destination $packageDir -Force
}

$outputName = "_core.$pythonTag.pyd"
$outputPath = Join-Path $packageDir $outputName
if (Test-Path -LiteralPath $outputPath) {
    Remove-Item -LiteralPath $outputPath -Force
}

$includeDirs = @(
    "python\src",
    "src",
    "src\app",
    "src\basics",
    "src\calculator",
    "src\exprTree",
    "src\solution",
    "src\logger",
    "src\cp",
    "src\distribution",
    "src\problem\core",
    "src\problem\define",
    "src\problem\manager",
    "src\problem\variable",
    "src\problem\objective",
    "src\problem\constrain",
    "src\problem\heuristic",
    "src\problem\template",
    "src\optimizer\core",
    "src\optimizer\individual",
    "src\optimizer\solution-op",
    "src\optimizer\population",
    "src\optimizer\subpopulation-constructer",
    "src\optimizer\subpopulation-topology",
    "src\optimizer\subpopulation-manager",
    "src\optimizer\strategy",
    "src\optimizer\topology",
    "src\optimizer\generator",
    "src\optimizer\selector",
    "src\optimizer\evaluator",
    "src\optimizer\terminator",
    "src\optimizer\archive",
    "src\optimizer\orchestrate",
    "src\optimizer\config",
    "src\multiobjective",
    "src\analyzer",
    "src\analyzer\out",
    "thirdparty\muparserx\include",
    "thirdparty\muparserx\src\parser",
    "thirdparty\sortlib\include",
    "thirdparty\randlib\include",
    "thirdparty\timelib\include",
    "thirdparty\doublelib\include",
    "thirdparty\distancelib\include",
    "thirdparty\filelib\include",
    "thirdparty\statslib\include",
    "thirdparty\stringlib\src\stringlib\stringlib",
    "thirdparty\metriclib\src\metriclib\metriclib",
    "thirdparty\stb",
    $pythonInclude,
    $pybindInclude
)

$sourceFiles = @(
    "python\src\module.cpp",
    "python\src\bind-app.cpp",
    "python\src\bind-problem.cpp",
    "python\src\bind-template.cpp",
    "python\src\bind-config.cpp",
    "python\src\bind-run.cpp",
    "python\src\bind-analysis.cpp",
    "python\src\probe-callback.cpp",
    "src\problem\variable\variable.cpp",
    "src\problem\objective\objective.cpp",
    "src\solution\solution.cpp",
    "src\solution\solution-decoder.cpp",
    "src\calculator\func-calculator.cpp",
    "src\calculator\tree-calculator.cpp",
    "src\exprTree\ExprTree.cpp",
    "src\app\app-facade.cpp",
    "src\app\component-catalog.cpp",
    "src\app\config-editor.cpp",
    "src\app\config-validator.cpp",
    "src\app\run-reporter.cpp",
    "src\analyzer\out\out-png.cpp"
)
$sourceFiles += Get-ChildItem -LiteralPath (Join-Path $branch "src\basics") -File -Filter "*.cpp" |
    ForEach-Object { $_.FullName.Substring($branch.Length + 1) }
$sourceFiles += Get-ChildItem -LiteralPath (Join-Path $branch "thirdparty\muparserx\src") -File -Filter "*.cpp" -Recurse |
    ForEach-Object { $_.FullName.Substring($branch.Length + 1) }
$response = @(
    "/std:c++17",
    "/utf-8",
    "/EHsc",
    "/bigobj",
    "/permissive-",
    "/nologo",
    "/W1",
    "/wd4828",
    "/O2",
    "/MD",
    "/LD",
    "/Fo$($objDir.Substring($branch.Length + 1))\",
    "/Fe$($outputPath.Substring($branch.Length + 1))"
)
if ($Parallel) {
    $response += "/DEVOFLOW_PARALLEL_SUBPOP"
}
$response += @($includeDirs | ForEach-Object { "/I`"$_`"" })
$response += @($sourceFiles | ForEach-Object { "`"$_`"" })
$response += @(
    "/link",
    "`"$pythonLib`"",
    "advapi32.lib",
    "ws2_32.lib",
    "shell32.lib",
    "bcrypt.lib",
    "winhttp.lib"
)

$responseFile = Join-Path $objRoot "build-$mode.rsp"
$response | Set-Content -LiteralPath $responseFile -Encoding ASCII

Write-Host "Building EvoFlow Python probe ($mode, $pythonTag)"
$command = "`"$vcvars`" >nul 2>&1 && cd /d `"$branch`" && cl @`"$responseFile`""
$oldTemp = $env:TEMP
$oldTmp = $env:TMP
try {
    $env:TEMP = $tempDir
    $env:TMP = $tempDir
    cmd /c $command
    $compilerExit = $LASTEXITCODE
}
finally {
    $env:TEMP = $oldTemp
    $env:TMP = $oldTmp
}
if ($compilerExit -ne 0 -or -not (Test-Path -LiteralPath $outputPath)) {
    throw "Python extension build failed"
}
$outputBase = Join-Path $packageDir ([System.IO.Path]::GetFileNameWithoutExtension($outputName))
foreach ($sidecar in @("$outputBase.lib", "$outputBase.exp")) {
    if (Test-Path -LiteralPath $sidecar) {
        Remove-Item -LiteralPath $sidecar -Force
    }
}

Write-Host "Built: $outputPath"
if (-not $SkipTests) {
    $oldPythonPath = $env:PYTHONPATH
    $oldNoBytecode = $env:PYTHONDONTWRITEBYTECODE
    try {
        $env:PYTHONPATH = $packageRoot
        $env:PYTHONDONTWRITEBYTECODE = "1"
        Push-Location $branch
        try {
            foreach ($test in @(
                "tests\python\smoke-import.py",
                "tests\python\smoke-run.py",
                "tests\python\smoke-runtime.py",
                "tests\python\smoke-callback-gil.py",
                "tests\python\smoke-problem-model.py",
                "tests\python\smoke-problem-constraints.py",
                "tests\python\smoke-problem-callback.py",
                "tests\python\smoke-config-model.py",
                "tests\python\smoke-config-catalog.py",
                "tests\python\smoke-analysis.py",
                "tests\python\smoke-analysis-output.py",
                "tests\python\smoke-template.py",
                "tests\python\smoke-instance-catalog.py",
                "tests\python\smoke-package.py",
                "tests\python\smoke-external-data.py",
                "tests\python\smoke-coverage.py",
                "tests\python\smoke-examples.py"
            )) {
                & $PythonExe $test
                if ($LASTEXITCODE -ne 0) {
                    throw "Python test failed: $test"
                }
            }
        }
        finally {
            Pop-Location
        }
    }
    finally {
        $env:PYTHONPATH = $oldPythonPath
        $env:PYTHONDONTWRITEBYTECODE = $oldNoBytecode
    }
}

Write-Host "Python binding probe build and tests: PASS"
