$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path "build" | Out-Null

$msysBin = "C:\msys64\ucrt64\bin"
if ((Test-Path $msysBin) -and -not ($env:PATH -split ";" | Where-Object { $_ -eq $msysBin })) {
    $env:PATH = "$msysBin;$env:PATH"
}

if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) { throw "GCC/MinGW-w64 nao encontrado no PATH." }
if (-not (Get-Command windres -ErrorAction SilentlyContinue)) { throw "windres nao encontrado. Instale o pacote MinGW-w64 completo." }

& powershell -ExecutionPolicy Bypass -File "scripts/build-raylib.ps1"
if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar Raylib/GLFW." }

& powershell -ExecutionPolicy Bypass -File "brand/build-icon.ps1"
if ($LASTEXITCODE -ne 0) { throw "Falha ao gerar o icone." }

$resource = '#define IDI_APP_ICON 101' + "`r`n" + 'IDI_APP_ICON ICON "build/cdeck.ico"' + "`r`n" + '1 24 "installer/C-Deck.manifest"' + "`r`n"
Set-Content -Path "build/cdeck.rc" -Value $resource -Encoding Ascii
windres build/cdeck.rc build/cdeck-res.o
if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar o recurso do icone." }

gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -D_WIN32_WINNT=0x0601 -Ibuild/deps/stage/include src/main.c src/interface.c src/audio.c src/apps.c build/cdeck-res.o -o build/C-Deck.exe -mwindows -Lbuild/deps/stage/lib -lraylib -lopengl32 -lgdi32 -lwinmm -luser32 -lshell32 -lole32 -luuid
if ($LASTEXITCODE -ne 0) { throw "Compilacao falhou." }

$stageBin = "build/deps/stage/bin"
foreach ($dllName in @("libraylib.dll", "glfw3.dll")) {
    $dllPath = Join-Path $stageBin $dllName
    if (-not (Test-Path $dllPath)) {
        throw "$dllName nao encontrado em $stageBin."
    }
    Copy-Item $dllPath (Join-Path "build" $dllName) -Force
}

$lddCmd = Get-Command ldd -ErrorAction SilentlyContinue
if (-not $lddCmd -and (Test-Path "C:\msys64\usr\bin\ldd.exe")) {
    $lddCmd = Get-Command "C:\msys64\usr\bin\ldd.exe"
}
if ($lddCmd) {
    $gccBin = Split-Path (Get-Command gcc).Source -Parent
    $binaries = @("build/C-Deck.exe") + @(Get-ChildItem "build/*.dll" | ForEach-Object { $_.FullName })
    foreach ($bin in $binaries) {
        $deps = & $lddCmd.Source $bin
        if ($deps | Where-Object { $_ -match 'not found' }) {
            $deps | Write-Host
            throw "Dependencia de runtime ausente em $bin"
        }
        foreach ($line in $deps) {
            if ($line -match '=>\s+([^\s]+/ucrt64/bin/[^\s]+\.dll)\s') {
                $winPath = $Matches[1] -replace '^/([a-zA-Z])/', '$1:/'
                if (Test-Path $winPath) {
                    Copy-Item $winPath "build/" -Force
                }
            } elseif ($line -match '=>\s+([^\s]+\\ucrt64\\bin\\[^\s]+\.dll)\s') {
                if (Test-Path $Matches[1]) {
                    Copy-Item $Matches[1] "build/" -Force
                }
            }
        }
    }
}

Copy-Item README.md build/README.md -Force
Write-Host "C-Deck com interface Raylib compilado: build/C-Deck.exe"
