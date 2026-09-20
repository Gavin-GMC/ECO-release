# Generates a C++ header that embeds the browser UI assets.
param([Parameter(Mandatory=$true)][string]$Output)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$assets = @(
    @{ File = "index.html"; Symbol = "indexHtml" },
    @{ File = "app.css"; Symbol = "appCss" },
    @{ File = "i18n.js"; Symbol = "i18nJs" },
    @{ File = "app.js"; Symbol = "appJs" },
    @{ File = "llm-settings.js"; Symbol = "llmSettingsJs" },
    @{ File = "problem-builder.js"; Symbol = "problemBuilderJs" },
    @{ File = "config-editor.js"; Symbol = "configEditorJs" },
    @{ File = "config-recommender.js"; Symbol = "configRecommenderJs" },
    @{ File = "flow-editor.js"; Symbol = "flowEditorJs" },
    @{ File = "report-viewer.js"; Symbol = "reportViewerJs" }
)

$builder = New-Object System.Text.StringBuilder
[void]$builder.AppendLine("#pragma once")
[void]$builder.AppendLine("namespace EvoFlow { namespace UI { namespace Assets {")
foreach ($asset in $assets) {
    $path = Join-Path (Join-Path $scriptRoot "assets") $asset.File
    $content = [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)
    if ($content.Contains(')EVO"')) {
        throw "Asset '$($asset.File)' contains the reserved raw-string terminator."
    }
    # MSVC rejects a single string literal above about 16 KiB (C2026). Emit adjacent
    # raw-string chunks; C++ concatenates them into the same constexpr char array.
    [void]$builder.Append("inline constexpr char $($asset.Symbol)[] = ")
    $chunkChars = 8000
    for ($offset = 0; $offset -lt $content.Length; $offset += $chunkChars) {
        $length = [Math]::Min($chunkChars, $content.Length - $offset)
        [void]$builder.Append("R`"EVO(")
        [void]$builder.Append($content.Substring($offset, $length))
        [void]$builder.Append(")EVO`" ")
    }
    [void]$builder.AppendLine(";")
}
[void]$builder.AppendLine("}}}")

$parent = Split-Path -Parent $Output
if ($parent) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($Output, $builder.ToString(), $utf8NoBom)
