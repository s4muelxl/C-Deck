$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$version = (Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root "VERSION")).Trim()

if ($version -notmatch '^\d+\.\d+\.\d+$') {
    throw "VERSION must contain one semantic version in MAJOR.MINOR.PATCH form."
}

$checks = @(
    @{ Path = "README.md"; Pattern = "Versão $version\." },
    @{ Path = "installer/C-Deck.iss"; Pattern = '#define AppVersion "' + [regex]::Escape($version) + '"' },
    @{ Path = "src/interface.c"; Pattern = "C-Deck v$([regex]::Escape($version)) · Código Aberto" },
    @{ Path = "web/index.html"; Pattern = "C-Deck Official Website v$([regex]::Escape($version))" },
    @{ Path = "web/index.html"; Pattern = "VERSÃO $([regex]::Escape($version))" }
)

foreach ($check in $checks) {
    $path = Join-Path $root $check.Path
    $text = Get-Content -Raw -Encoding UTF8 -LiteralPath $path
    if (-not [regex]::IsMatch($text, $check.Pattern)) {
        throw "Version mismatch: $($check.Path) does not declare $version."
    }
}

Write-Host "C-Deck version metadata is consistent: $version"
