$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path "build" | Out-Null
if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) { throw "GCC/MinGW-w64 nao encontrado no PATH." }
if (-not (Get-Command windres -ErrorAction SilentlyContinue)) { throw "windres nao encontrado." }
& powershell -ExecutionPolicy Bypass -File "brand/build-icon.ps1"
if ($LASTEXITCODE -ne 0) { throw "Falha ao gerar o icone." }
$resource = '#define IDI_APP_ICON 101' + [Environment]::NewLine + 'IDI_APP_ICON ICON "build/cdeck.ico"' + [Environment]::NewLine + '1 24 "installer/C-Deck.manifest"' + [Environment]::NewLine
Set-Content -Path "build/cdeck.rc" -Value $resource -Encoding Ascii
windres build/cdeck.rc build/cdeck-res.o
if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar recursos." }
gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -D_WIN32_WINNT=0x0601 src/simple_deck.c build/cdeck-res.o -o build/C-Deck.exe -mwindows -luser32 -lgdi32 -lshell32 -lole32 -luuid
if ($LASTEXITCODE -ne 0) { throw "Compilacao falhou." }
Write-Host "Build concluido: build/C-Deck.exe"
