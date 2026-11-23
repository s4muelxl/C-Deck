$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path "build/config" | Out-Null
if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) { throw "GCC/MinGW-w64 nao encontrado no PATH." }
if (-not (Get-Command windres -ErrorAction SilentlyContinue)) { throw "windres nao encontrado. Instale o pacote MinGW-w64 completo." }

& powershell -ExecutionPolicy Bypass -File "brand/build-icon.ps1"
if ($LASTEXITCODE -ne 0) { throw "Falha ao gerar o icone oficial." }
$resource = '#define IDI_APP_ICON 101' + "`r`n" + 'IDI_APP_ICON ICON "build/cdeck.ico"' + "`r`n" + '1 24 "installer/C-Deck.manifest"' + "`r`n"
Set-Content -Path "build/cdeck.rc" -Value $resource -Encoding Ascii
windres build/cdeck.rc build/cdeck-res.o
if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar o recurso de icone." }

gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -D_WIN32_WINNT=0x0601 src/main.c build/cdeck-res.o -o build/C-Deck.exe -mwindows -luser32 -lshell32 -lole32 -luuid
if ($LASTEXITCODE -ne 0) { throw "Compilacao falhou." }
Copy-Item config/apps.cfg build/config/apps.cfg -Force
Copy-Item config/macros.cfg.example build/config/macros.cfg.example -Force
Copy-Item README.md build/README.md -Force
Write-Host "Build academico concluido: build/C-Deck.exe"
