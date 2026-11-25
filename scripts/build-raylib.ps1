param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$depsDir = Join-Path $root "build\deps"
$downloadsDir = Join-Path $depsDir "downloads"
$srcDir = Join-Path $depsDir "src"
$stageDir = Join-Path $depsDir "stage"

$msysBin = "C:\msys64\ucrt64\bin"
if ((Test-Path $msysBin) -and -not ($env:PATH -split ";" | Where-Object { $_ -eq $msysBin })) {
    $env:PATH = "$msysBin;$env:PATH"
}

foreach ($tool in @("gcc", "cmake", "ninja")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "Ferramenta obrigatoria nao encontrada no PATH: $tool"
    }
}

$stagedFiles = @(
    (Join-Path $stageDir "bin\glfw3.dll"),
    (Join-Path $stageDir "bin\libraylib.dll"),
    (Join-Path $stageDir "lib\libraylib.dll.a"),
    (Join-Path $stageDir "include\raylib.h")
)

if (-not $Force -and (@($stagedFiles | Where-Object { -not (Test-Path $_) }).Count -eq 0)) {
    Write-Host "Raylib (OPENGL_VERSION=1.1) e GLFW ja compilados em $stageDir"
    exit 0
}

function Get-PinnedArchive {
    param(
        [string]$Url,
        [string]$Destination,
        [string]$ExpectedSha256
    )

    if (Test-Path $Destination) {
        $existingHash = (Get-FileHash -Path $Destination -Algorithm SHA256).Hash.ToUpperInvariant()
        if ($existingHash -eq $ExpectedSha256) {
            return
        }
        Remove-Item -LiteralPath $Destination -Force
    }

    Write-Host "Baixando $Url..."
    Invoke-WebRequest -Uri $Url -OutFile $Destination -UseBasicParsing
    $actualHash = (Get-FileHash -Path $Destination -Algorithm SHA256).Hash.ToUpperInvariant()
    if ($actualHash -ne $ExpectedSha256) {
        Remove-Item -LiteralPath $Destination -Force -ErrorAction SilentlyContinue
        throw "Checksum SHA-256 invalido para $Destination. Esperado: $ExpectedSha256, obtido: $actualHash"
    }
}

function Set-GlfwSoftwareFallback {
    param([string]$WglFilePath)

    if (-not (Test-Path $WglFilePath)) {
        throw "Arquivo esperado do GLFW nao encontrado: $WglFilePath"
    }

    $content = [System.IO.File]::ReadAllText($WglFilePath)
    if ($content.Contains("allowSoftware")) {
        return
    }

    $newline = if ($content.Contains("`r`n")) { "`r`n" } else { "`n" }
    $blocks = @(
        @{
            Target = "    usableConfigs = _glfw_calloc(nativeCount, sizeof(_GLFWfbconfig));`n`n    for (i = 0;  i < nativeCount;  i++)"
            Replacement = "    int allowSoftware;`n`n    usableConfigs = _glfw_calloc(nativeCount, sizeof(_GLFWfbconfig));`n`n    for (allowSoftware = 0;  allowSoftware <= 1 && !usableCount;  allowSoftware++)`n    {`n    for (i = 0;  i < nativeCount;  i++)"
        },
        @{
            Target = "            if (FIND_ATTRIB_VALUE(WGL_ACCELERATION_ARB) == WGL_NO_ACCELERATION_ARB)`n                continue;"
            Replacement = "            if (!allowSoftware &&`n                FIND_ATTRIB_VALUE(WGL_ACCELERATION_ARB) == WGL_NO_ACCELERATION_ARB)`n                continue;"
        },
        @{
            Target = "            if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) &&`n                (pfd.dwFlags & PFD_GENERIC_FORMAT))`n            {`n                continue;`n            }"
            Replacement = "            if (!allowSoftware &&`n                !(pfd.dwFlags & PFD_GENERIC_ACCELERATED) &&`n                (pfd.dwFlags & PFD_GENERIC_FORMAT))`n            {`n                continue;`n            }"
        },
        @{
            Target = "        u->handle = pixelFormat;`n        usableCount++;`n    }`n`n    if (!usableCount)"
            Replacement = "        u->handle = pixelFormat;`n        usableCount++;`n    }`n    }`n`n    if (!usableCount)"
        }
    )

    $normalized = $content.Replace("`r`n", "`n")
    foreach ($block in $blocks) {
        if (-not $normalized.Contains($block.Target)) {
            throw "Nao foi possivel aplicar o patch WGL fallback em $WglFilePath (trecho esperado ausente)."
        }
        $normalized = $normalized.Replace($block.Target, $block.Replacement)
    }

    if ($newline -eq "`r`n") {
        $normalized = $normalized.Replace("`n", "`r`n")
    }
    [System.IO.File]::WriteAllText($WglFilePath, $normalized, (New-Object System.Text.UTF8Encoding($false)))
    Write-Host "Patch WGL software fallback aplicado com sucesso em $WglFilePath"
}

New-Item -ItemType Directory -Force -Path $downloadsDir, $srcDir, $stageDir | Out-Null

$glfwArchive = Join-Path $downloadsDir "glfw-3.4.tar.gz"
$raylibArchive = Join-Path $downloadsDir "raylib-5.5.tar.gz"

Get-PinnedArchive -Url "https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz" -Destination $glfwArchive -ExpectedSha256 "C038D34200234D071FAE9345BC455E4A8F2F544AB60150765D7704E08F3DAC01"
Get-PinnedArchive -Url "https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz" -Destination $raylibArchive -ExpectedSha256 "AEA98ECF5BC5C5E0B789A76DE0083A21A70457050EA4CC2AEC7566935F5E258E"

$glfwSrc = Join-Path $srcDir "glfw-3.4"
$raylibSrc = Join-Path $srcDir "raylib-5.5"

Push-Location $srcDir
try {
    if (-not (Test-Path (Join-Path $glfwSrc "CMakeLists.txt"))) {
        & cmake -E tar xzf $glfwArchive
        if ($LASTEXITCODE -ne 0) { throw "Falha ao extrair GLFW 3.4." }
    }

    if (-not (Test-Path (Join-Path $raylibSrc "CMakeLists.txt"))) {
        & cmake -E tar xzf $raylibArchive
        if ($LASTEXITCODE -ne 0) { throw "Falha ao extrair Raylib 5.5." }
    }
} finally {
    Pop-Location
}

Set-GlfwSoftwareFallback -WglFilePath (Join-Path $glfwSrc "src\wgl_context.c")

$glfwBuild = Join-Path $depsDir "glfw-build"
$raylibBuild = Join-Path $depsDir "raylib-build"

Write-Host "Compilando GLFW 3.4 com fallback WGL..."
& cmake -S $glfwSrc -B $glfwBuild -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF "-DCMAKE_INSTALL_PREFIX=$stageDir"
if ($LASTEXITCODE -ne 0) { throw "Falha na configuracao CMake do GLFW." }
& cmake --build $glfwBuild
if ($LASTEXITCODE -ne 0) { throw "Falha na compilacao do GLFW." }
& cmake --install $glfwBuild
if ($LASTEXITCODE -ne 0) { throw "Falha na instalacao staged do GLFW." }

Write-Host "Compilando Raylib 5.5 com OPENGL_VERSION=1.1 (compativel com GPUs antigas e GDI Generic)..."
& cmake -S $raylibSrc -B $raylibBuild -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON "-DOPENGL_VERSION=1.1" -DUSE_EXTERNAL_GLFW=ON -DBUILD_EXAMPLES=OFF "-DCMAKE_PREFIX_PATH=$stageDir" "-DCMAKE_INSTALL_PREFIX=$stageDir"
if ($LASTEXITCODE -ne 0) { throw "Falha na configuracao CMake do Raylib." }
& cmake --build $raylibBuild
if ($LASTEXITCODE -ne 0) { throw "Falha na compilacao do Raylib." }
& cmake --install $raylibBuild
if ($LASTEXITCODE -ne 0) { throw "Falha na instalacao staged do Raylib." }

foreach ($file in $stagedFiles) {
    if (-not (Test-Path $file)) {
        throw "Artefato esperado ausente apos compilacao das dependencias: $file"
    }
}

Write-Host "Dependencias graficas compiladas com sucesso em $stageDir"
