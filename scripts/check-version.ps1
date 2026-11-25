$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$version = (Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $root "VERSION")).Trim()

if ($version -notmatch '^\d+\.\d+\.\d+$') {
    throw "VERSION must use MAJOR.MINOR.PATCH format."
}

$versaoWord = "Vers$([char]0x00E3)o"
$versaoUpper = "VERS$([char]0x00C3)O"
$checks = @(
    @{ Path = "README.md"; Pattern = "$versaoWord $([regex]::Escape($version))\." },
    @{ Path = "installer/C-Deck.iss"; Pattern = '#define AppVersion "' + [regex]::Escape($version) + '"' },
    @{ Path = "web/index.html"; Pattern = "C-Deck Official Website v$([regex]::Escape($version))" },
    @{ Path = "web/index.html"; Pattern = "$versaoUpper $([regex]::Escape($version))" }
)

foreach ($check in $checks) {
    $path = Join-Path $root $check.Path
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $path
    if (-not [regex]::IsMatch($content, $check.Pattern)) {
        throw "Version mismatch: $($check.Path) does not declare $version."
    }
}

foreach ($source in @("src/main.c", "src/interface.c", "src/interface.h", "src/audio.c", "src/audio.h", "src/apps.c", "src/apps.h")) {
    if (-not (Test-Path (Join-Path $root $source))) {
        throw "Required source file missing: $source"
    }
}

Write-Host "C-Deck version and source structure are consistent: $version"
