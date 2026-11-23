$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$version = (Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root "VERSION")).Trim()

if ($version -notmatch '^\d+\.\d+\.\d+$') {
    throw "VERSION must use MAJOR.MINOR.PATCH format."
}

$checks = @(
    @{ Path = "README.md"; Pattern = "Versão $([regex]::Escape($version))\." },
    @{ Path = "installer/C-Deck.iss"; Pattern = '#define AppVersion "' + [regex]::Escape($version) + '"' },
    @{ Path = "web/index.html"; Pattern = "C-Deck Official Website v$([regex]::Escape($version))" },
    @{ Path = "web/index.html"; Pattern = "VERSÃO $([regex]::Escape($version))" }
)

foreach ($check in $checks) {
    $path = Join-Path $root $check.Path
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $path
    if (-not [regex]::IsMatch($content, $check.Pattern)) {
        throw "Version mismatch: $($check.Path) does not declare $version."
    }
}

Write-Host "C-Deck version metadata is consistent: $version"
