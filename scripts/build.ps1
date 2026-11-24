$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path "build" | Out-Null
if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) { throw "GCC/MinGW-w64 nao encontrado no PATH." }
if (-not (Get-Command windres -ErrorAction SilentlyContinue)) { throw "windres nao encontrado. Instale o pacote MinGW-w64 completo." }

& powershell -ExecutionPolicy Bypass -File "brand/build-icon.ps1"
if ($LASTEXITCODE -ne 0) { throw "Falha ao gerar o icone." }

$resource = '#define IDI_APP_ICON 101' + "`r`n" + 'IDI_APP_ICON ICON "build/cdeck.ico"' + "`r`n" + '1 24 "installer/C-Deck.manifest"' + "`r`n"
Set-Content -Path "build/cdeck.rc" -Value $resource -Encoding Ascii
windres build/cdeck.rc build/cdeck-res.o
if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar o recurso do icone." }

gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -D_WIN32_WINNT=0x0601 src/main.c src/interface.c src/audio.c src/apps.c build/cdeck-res.o -o build/C-Deck.exe -mwindows -lraylib -lopengl32 -lgdi32 -lwinmm -luser32 -lshell32 -lole32 -luuid
if ($LASTEXITCODE -ne 0) { throw "Compilacao falhou." }

$raylibDll = Join-Path (Split-Path (Get-Command gcc).Source -Parent) "libraylib.dll"
if (Test-Path $raylibDll) {
    Copy-Item $raylibDll "build/libraylib.dll" -Force
} else {
    throw "libraylib.dll nao encontrado. Instale mingw-w64-ucrt-x86_64-raylib no MSYS2 UCRT64."
}

Copy-Item README.md build/README.md -Force
Write-Host "C-Deck com interface Raylib compilado: build/C-Deck.exe"
