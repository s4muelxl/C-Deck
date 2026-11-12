$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path "build" | Out-Null
if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) { throw "GCC/MinGW-w64 nao encontrado no PATH." }
gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -D_WIN32_WINNT=0x0601 src/main.c src/interface.c src/actions.c src/config.c -o build/C-Deck.exe -mwindows -luser32 -lgdi32 -lshell32 -lole32 -luuid -lcomctl32
if ($LASTEXITCODE -ne 0) { throw "Compilacao falhou." }
Copy-Item config/apps.cfg build/apps.cfg -Force
Write-Host "Build concluido: build/C-Deck.exe"
