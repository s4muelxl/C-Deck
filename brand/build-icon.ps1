$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildDir = Join-Path $root "build"
$brandDir = Join-Path $root "brand"
$srcDir = Join-Path $root "src"
New-Item -ItemType Directory -Force -Path $buildDir, $brandDir | Out-Null

function New-RoundedRectPath {
    param(
        [float]$X,
        [float]$Y,
        [float]$Width,
        [float]$Height,
        [float]$Radius
    )
    $d = $Radius * 2.0
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddArc($X, $Y, $d, $d, 180.0, 90.0)
    $path.AddArc($X + $Width - $d, $Y, $d, $d, 270.0, 90.0)
    $path.AddArc($X + $Width - $d, $Y + $Height - $d, $d, $d, 0.0, 90.0)
    $path.AddArc($X, $Y + $Height - $d, $d, $d, 90.0, 90.0)
    $path.CloseFigure()
    return $path
}

function Render-CDeckMasterBitmap {
    param(
        [int]$TargetSize,
        [switch]$UiHeaderTile
    )

    $master = 1024
    $bmp = New-Object System.Drawing.Bitmap($master, $master, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)

    $drawShadow = $false
    if ($UiHeaderTile) {
        $margin = 20.0
        $radius = 212.0
        $rimWidth = 22.0
        $cScale = 1.04
    } elseif ($TargetSize -le 24) {
        $margin = 24.0
        $radius = 220.0
        $rimWidth = 44.0
        $cScale = 1.08
    } elseif ($TargetSize -le 48) {
        $margin = 40.0
        $radius = 204.0
        $rimWidth = 26.0
        $cScale = 1.03
    } else {
        $margin = 56.0
        $radius = 192.0
        $rimWidth = 10.0
        $cScale = 1.00
        $drawShadow = $true
    }

    $tileSize = 1024.0 - ($margin * 2.0)

    if ($drawShadow) {
        $shadowSteps = @(
            @{ Expand = 24.0; OffsetY = 14.0; Alpha = 10 },
            @{ Expand = 16.0; OffsetY = 11.0; Alpha = 18 },
            @{ Expand = 8.0;  OffsetY = 8.0;  Alpha = 28 },
            @{ Expand = 2.0;  OffsetY = 4.0;  Alpha = 40 }
        )
        foreach ($step in $shadowSteps) {
            $sp = New-RoundedRectPath ($margin - $step.Expand) ($margin - $step.Expand + $step.OffsetY) ($tileSize + $step.Expand * 2.0) ($tileSize + $step.Expand * 2.0) ($radius + $step.Expand * 0.5)
            $sb = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb($step.Alpha, 0, 0, 0))
            $g.FillPath($sb, $sp)
            $sb.Dispose()
            $sp.Dispose()
        }

        $limeGlowPath = New-Object System.Drawing.Drawing2D.GraphicsPath
        $limeGlowPath.AddEllipse([float]($margin + $tileSize * 0.48), [float]($margin + $tileSize * 0.48), [float]($tileSize * 0.62), [float]($tileSize * 0.62))
        $limeGlowBrush = New-Object System.Drawing.Drawing2D.PathGradientBrush($limeGlowPath)
        $limeGlowBrush.CenterColor = [System.Drawing.Color]::FromArgb(24, 157, 255, 0)
        $limeGlowBrush.SurroundColors = [System.Drawing.Color[]]@([System.Drawing.Color]::FromArgb(0, 157, 255, 0))
        $g.FillPath($limeGlowBrush, $limeGlowPath)
        $limeGlowBrush.Dispose()
        $limeGlowPath.Dispose()
    }

    $tilePath = New-RoundedRectPath $margin $margin $tileSize $tileSize $radius

    # Base obsidian tile gradient
    $tileBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        [System.Drawing.PointF]::new($margin, $margin),
        [System.Drawing.PointF]::new($margin + $tileSize, $margin + $tileSize),
        [System.Drawing.Color]::FromArgb(255, 30, 36, 47),
        [System.Drawing.Color]::FromArgb(255, 9, 11, 15)
    )
    $tileBlend = New-Object System.Drawing.Drawing2D.ColorBlend
    $tileBlend.Positions = [float[]]@(0.0, 0.56, 1.0)
    $tileBlend.Colors = [System.Drawing.Color[]]@(
        [System.Drawing.Color]::FromArgb(255, 31, 37, 48),
        [System.Drawing.Color]::FromArgb(255, 13, 16, 22),
        [System.Drawing.Color]::FromArgb(255, 9, 11, 14)
    )
    $tileBrush.InterpolationColors = $tileBlend
    $g.FillPath($tileBrush, $tilePath)
    $tileBrush.Dispose()

    # Clipped interior lighting & bevel
    $g.SetClip($tilePath)

    # Subtle bottom-right lime ambient reflection inside keycap
    $innerLimePath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $innerLimePath.AddEllipse([float]($margin + $tileSize * 0.34), [float]($margin + $tileSize * 0.36), [float]($tileSize * 0.92), [float]($tileSize * 0.92))
    $innerLimeBrush = New-Object System.Drawing.Drawing2D.PathGradientBrush($innerLimePath)
    $innerLimeBrush.CenterColor = [System.Drawing.Color]::FromArgb(34, 157, 255, 0)
    $innerLimeBrush.SurroundColors = [System.Drawing.Color[]]@([System.Drawing.Color]::FromArgb(0, 157, 255, 0))
    $g.FillPath($innerLimeBrush, $innerLimePath)
    $innerLimeBrush.Dispose()
    $innerLimePath.Dispose()

    # Subtle top specular sheen
    $sheenRect = [System.Drawing.RectangleF]::new($margin, $margin, $tileSize, $tileSize * 0.42)
    $sheenBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        [System.Drawing.PointF]::new($margin, $margin),
        [System.Drawing.PointF]::new($margin, $margin + $tileSize * 0.42),
        [System.Drawing.Color]::FromArgb(24, 255, 255, 255),
        [System.Drawing.Color]::FromArgb(0, 255, 255, 255)
    )
    $g.FillRectangle($sheenBrush, $sheenRect)
    $sheenBrush.Dispose()

    # Inner recessed ring for hardware keycap depth (for medium/large icons)
    if ($TargetSize -ge 32 -or $UiHeaderTile) {
        $inset = $rimWidth * 1.35
        $innerPath = New-RoundedRectPath ($margin + $inset) ($margin + $inset) ($tileSize - $inset * 2.0) ($tileSize - $inset * 2.0) ([Math]::Max(8.0, $radius - $inset))
        $innerPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(140, 4, 5, 8), [float]([Math]::Max(4.0, $rimWidth * 0.65)))
        $g.DrawPath($innerPen, $innerPath)
        $innerPen.Dispose()
        $innerPath.Dispose()
    }

    # Subtle lime bloom behind the lime blade
    if ($TargetSize -ge 48 -or $UiHeaderTile) {
        $bladeGlow = New-Object System.Drawing.Drawing2D.GraphicsPath
        $bladeGlow.AddEllipse(470.0, 560.0, 400.0, 310.0)
        $bladeGlowBrush = New-Object System.Drawing.Drawing2D.PathGradientBrush($bladeGlow)
        $bladeGlowBrush.CenterColor = [System.Drawing.Color]::FromArgb(30, 157, 255, 0)
        $bladeGlowBrush.SurroundColors = [System.Drawing.Color[]]@([System.Drawing.Color]::FromArgb(0, 157, 255, 0))
        $g.FillPath($bladeGlowBrush, $bladeGlow)
        $bladeGlowBrush.Dispose()
        $bladeGlow.Dispose()
    }

    # Geometric faceted 'C' + electric lime blade
    $savedState = $g.Save()
    $g.TranslateTransform(512.0, 512.0)
    $g.ScaleTransform($cScale, $cScale)
    $g.TranslateTransform(-512.0, -512.0)

    # Silver C polygon extends slightly under the lime miter to guarantee zero antialias seam
    $cPoints = [System.Drawing.PointF[]]@(
        [System.Drawing.PointF]::new(724.0, 248.0),
        [System.Drawing.PointF]::new(402.0, 248.0),
        [System.Drawing.PointF]::new(238.0, 412.0),
        [System.Drawing.PointF]::new(238.0, 612.0),
        [System.Drawing.PointF]::new(402.0, 776.0),
        [System.Drawing.PointF]::new(566.0, 776.0),
        [System.Drawing.PointF]::new(682.0, 660.0),
        [System.Drawing.PointF]::new(450.0, 660.0),
        [System.Drawing.PointF]::new(354.0, 564.0),
        [System.Drawing.PointF]::new(354.0, 460.0),
        [System.Drawing.PointF]::new(450.0, 364.0),
        [System.Drawing.PointF]::new(724.0, 364.0)
    )
    $silverBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        [System.Drawing.PointF]::new(238.0, 248.0),
        [System.Drawing.PointF]::new(724.0, 776.0),
        [System.Drawing.Color]::White,
        [System.Drawing.Color]::FromArgb(255, 180, 188, 201)
    )
    $silverBlend = New-Object System.Drawing.Drawing2D.ColorBlend
    $silverBlend.Positions = [float[]]@(0.0, 0.45, 0.78, 1.0)
    $silverBlend.Colors = [System.Drawing.Color[]]@(
        [System.Drawing.Color]::FromArgb(255, 255, 255, 255),
        [System.Drawing.Color]::FromArgb(255, 236, 239, 245),
        [System.Drawing.Color]::FromArgb(255, 180, 188, 202),
        [System.Drawing.Color]::FromArgb(255, 232, 236, 242)
    )
    $silverBrush.InterpolationColors = $silverBlend
    $g.FillPolygon($silverBrush, $cPoints)
    $silverBrush.Dispose()

    # Facet highlight along upper-left bevel
    $topFacet = [System.Drawing.PointF[]]@(
        [System.Drawing.PointF]::new(238.0, 412.0),
        [System.Drawing.PointF]::new(402.0, 248.0),
        [System.Drawing.PointF]::new(724.0, 248.0),
        [System.Drawing.PointF]::new(678.0, 294.0),
        [System.Drawing.PointF]::new(424.0, 294.0),
        [System.Drawing.PointF]::new(284.0, 434.0),
        [System.Drawing.PointF]::new(284.0, 588.0),
        [System.Drawing.PointF]::new(238.0, 612.0)
    )
    $topFacetBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(44, 255, 255, 255))
    $g.FillPolygon($topFacetBrush, $topFacet)
    $topFacetBrush.Dispose()

    # Subtle lower bevel shadow
    $lowFacet = [System.Drawing.PointF[]]@(
        [System.Drawing.PointF]::new(238.0, 612.0),
        [System.Drawing.PointF]::new(402.0, 776.0),
        [System.Drawing.PointF]::new(560.0, 776.0),
        [System.Drawing.PointF]::new(608.0, 728.0),
        [System.Drawing.PointF]::new(424.0, 728.0),
        [System.Drawing.PointF]::new(284.0, 588.0)
    )
    $lowFacetBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(30, 52, 62, 78))
    $g.FillPolygon($lowFacetBrush, $lowFacet)
    $lowFacetBrush.Dispose()

    # Signature #9DFF00 lime angled blade
    $limePoints = [System.Drawing.PointF[]]@(
        [System.Drawing.PointF]::new(542.0, 776.0),
        [System.Drawing.PointF]::new(678.0, 776.0),
        [System.Drawing.PointF]::new(794.0, 660.0),
        [System.Drawing.PointF]::new(658.0, 660.0)
    )
    $limeBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        [System.Drawing.PointF]::new(542.0, 660.0),
        [System.Drawing.PointF]::new(794.0, 776.0),
        [System.Drawing.Color]::FromArgb(255, 184, 255, 62),
        [System.Drawing.Color]::FromArgb(255, 126, 224, 0)
    )
    $limeBlend = New-Object System.Drawing.Drawing2D.ColorBlend
    $limeBlend.Positions = [float[]]@(0.0, 0.5, 1.0)
    $limeBlend.Colors = [System.Drawing.Color[]]@(
        [System.Drawing.Color]::FromArgb(255, 186, 255, 66),
        [System.Drawing.Color]::FromArgb(255, 157, 255, 0),
        [System.Drawing.Color]::FromArgb(255, 122, 220, 0)
    )
    $limeBrush.InterpolationColors = $limeBlend
    $g.FillPolygon($limeBrush, $limePoints)
    $limeBrush.Dispose()

    # Subtle specular edge on top of lime blade
    $limeEdge = [System.Drawing.PointF[]]@(
        [System.Drawing.PointF]::new(658.0, 660.0),
        [System.Drawing.PointF]::new(794.0, 660.0),
        [System.Drawing.PointF]::new(774.0, 680.0),
        [System.Drawing.PointF]::new(638.0, 680.0)
    )
    $limeEdgeBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(40, 255, 255, 255))
    $g.FillPolygon($limeEdgeBrush, $limeEdge)
    $limeEdgeBrush.Dispose()

    $g.Restore($savedState)
    $g.ResetClip()

    # Metallic-to-lime gradient rim stroke
    $rimBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        [System.Drawing.PointF]::new($margin, $margin),
        [System.Drawing.PointF]::new($margin + $tileSize, $margin + $tileSize),
        [System.Drawing.Color]::FromArgb(220, 186, 200, 222),
        [System.Drawing.Color]::FromArgb(230, 165, 255, 22)
    )
    $rimBlend = New-Object System.Drawing.Drawing2D.ColorBlend
    $rimBlend.Positions = [float[]]@(0.0, 0.54, 0.84, 1.0)
    $rimBlend.Colors = [System.Drawing.Color[]]@(
        [System.Drawing.Color]::FromArgb(220, 188, 202, 224),
        [System.Drawing.Color]::FromArgb(190, 66, 75, 92),
        [System.Drawing.Color]::FromArgb(215, 128, 198, 26),
        [System.Drawing.Color]::FromArgb(240, 165, 255, 22)
    )
    $rimBrush.InterpolationColors = $rimBlend
    $rimPen = New-Object System.Drawing.Pen($rimBrush, $rimWidth)
    $rimPen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round
    $g.DrawPath($rimPen, $tilePath)

    $rimPen.Dispose()
    $rimBrush.Dispose()
    $tilePath.Dispose()
    $g.Dispose()
    return $bmp
}

function Resize-HighQuality {
    param(
        [System.Drawing.Bitmap]$Source,
        [int]$Width,
        [int]$Height
    )
    if ($Source.Width -eq $Width -and $Source.Height -eq $Height) {
        return $Source
    }
    $dest = New-Object System.Drawing.Bitmap($Width, $Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($dest)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.DrawImage($Source, 0, 0, $Width, $Height)
    $g.Dispose()
    return $dest
}

function Convert-BitmapToPngBytes {
    param([System.Drawing.Bitmap]$Bitmap)
    $ms = New-Object System.IO.MemoryStream
    $Bitmap.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $bytes = $ms.ToArray()
    $ms.Dispose()
    return $bytes
}

function Convert-BitmapToIcoDibBytes {
    param([System.Drawing.Bitmap]$Bitmap)
    $w = $Bitmap.Width
    $h = $Bitmap.Height

    $rect = [System.Drawing.Rectangle]::new(0, 0, $w, $h)
    $bits = $Bitmap.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $stride = [Math]::Abs($bits.Stride)
    $raw = New-Object byte[] ($stride * $h)
    [System.Runtime.InteropServices.Marshal]::Copy($bits.Scan0, $raw, 0, $raw.Length)
    $Bitmap.UnlockBits($bits)

    $ms = New-Object System.IO.MemoryStream
    $bw = New-Object System.IO.BinaryWriter($ms)

    # BITMAPINFOHEADER (40 bytes)
    $bw.Write([UInt32]40)
    $bw.Write([Int32]$w)
    $bw.Write([Int32]($h * 2))
    $bw.Write([UInt16]1)
    $bw.Write([UInt16]32)
    $bw.Write([UInt32]0)
    $bw.Write([UInt32]($w * $h * 4))
    $bw.Write([Int32]0)
    $bw.Write([Int32]0)
    $bw.Write([UInt32]0)
    $bw.Write([UInt32]0)

    # XOR pixels (bottom-up BGRA)
    for ($y = $h - 1; $y -ge 0; $y--) {
        $bw.Write($raw, $y * $stride, $w * 4)
    }

    # 1bpp AND mask (bottom-up, DWORD aligned rows)
    $andRowBytes = [int]([Math]::Ceiling($w / 32.0) * 4)
    for ($y = $h - 1; $y -ge 0; $y--) {
        $rowMask = New-Object byte[] $andRowBytes
        $rowOffset = $y * $stride
        for ($x = 0; $x -lt $w; $x++) {
            $alpha = $raw[$rowOffset + ($x * 4) + 3]
            if ($alpha -eq 0) {
                $byteIndex = [int][Math]::Floor($x / 8)
                $bitIndex = 7 - ($x % 8)
                $rowMask[$byteIndex] = $rowMask[$byteIndex] -bor (1 -shl $bitIndex)
            }
        }
        $bw.Write($rowMask, 0, $andRowBytes)
    }

    $bw.Flush()
    $bytes = $ms.ToArray()
    $bw.Dispose()
    $ms.Dispose()
    return $bytes
}

# 1. Generate multi-resolution Windows ICO (16..128 DIB + 256 PNG)
$icoSizes = @(16, 20, 24, 32, 40, 48, 64, 96, 128, 256)
$entries = @()
$masterLarge = $null

foreach ($sz in $icoSizes) {
    if ($sz -ge 64) {
        if ($null -eq $masterLarge) {
            $masterLarge = Render-CDeckMasterBitmap -TargetSize 256
        }
        $scaled = Resize-HighQuality -Source $masterLarge -Width $sz -Height $sz
    } else {
        $masterSized = Render-CDeckMasterBitmap -TargetSize $sz
        $scaled = Resize-HighQuality -Source $masterSized -Width $sz -Height $sz
        $masterSized.Dispose()
    }

    if ($sz -eq 256) {
        $payload = Convert-BitmapToPngBytes -Bitmap $scaled
    } else {
        $payload = Convert-BitmapToIcoDibBytes -Bitmap $scaled
    }
    $entries += [PSCustomObject]@{
        Size = $sz
        Data = $payload
    }
    $scaled.Dispose()
}

$icoStream = New-Object System.IO.MemoryStream
$icoWriter = New-Object System.IO.BinaryWriter($icoStream)
$icoWriter.Write([UInt16]0)
$icoWriter.Write([UInt16]1)
$icoWriter.Write([UInt16]$entries.Count)

$offset = 6 + (16 * $entries.Count)
foreach ($entry in $entries) {
    $dim = if ($entry.Size -ge 256) { [Byte]0 } else { [Byte]$entry.Size }
    $icoWriter.Write($dim)
    $icoWriter.Write($dim)
    $icoWriter.Write([Byte]0)
    $icoWriter.Write([Byte]0)
    $icoWriter.Write([UInt16]1)
    $icoWriter.Write([UInt16]32)
    $icoWriter.Write([UInt32]$entry.Data.Length)
    $icoWriter.Write([UInt32]$offset)
    $offset += $entry.Data.Length
}
foreach ($entry in $entries) {
    $icoWriter.Write($entry.Data)
}
$icoWriter.Flush()
$icoBytes = $icoStream.ToArray()
$icoWriter.Dispose()
$icoStream.Dispose()

[IO.File]::WriteAllBytes((Join-Path $buildDir "cdeck.ico"), $icoBytes)

# 2. Save 1024x1024 master PNG for inspection & branding
$masterPngBytes = Convert-BitmapToPngBytes -Bitmap $masterLarge
[IO.File]::WriteAllBytes((Join-Path $brandDir "cdeck-icon.png"), $masterPngBytes)
$masterLarge.Dispose()

# 3. Generate embedded 64x64 POT PNG (44x44 supersampled tile at origin) for Raylib UI header
$uiMaster = Render-CDeckMasterBitmap -TargetSize 44 -UiHeaderTile
$uiScaled = Resize-HighQuality -Source $uiMaster -Width 44 -Height 44
$uiMaster.Dispose()

$uiPot = New-Object System.Drawing.Bitmap(64, 64, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$uiG = [System.Drawing.Graphics]::FromImage($uiPot)
$uiG.Clear([System.Drawing.Color]::Transparent)
$uiG.DrawImage($uiScaled, 0, 0, 44, 44)
$uiG.Dispose()
$uiScaled.Dispose()

$uiPngBytes = Convert-BitmapToPngBytes -Bitmap $uiPot
$uiPot.Dispose()

$sb = New-Object System.Text.StringBuilder
[void]$sb.Append("/* Auto-generated by brand/build-icon.ps1 - do not edit manually. */`n")
[void]$sb.Append("#ifndef CDECK_ICON_DATA_H`n")
[void]$sb.Append("#define CDECK_ICON_DATA_H`n`n")
[void]$sb.Append("#define CDECK_UI_ICON_TEX_SIZE 64`n")
[void]$sb.Append("#define CDECK_UI_ICON_DRAW_SIZE 44`n`n")
[void]$sb.Append("static const unsigned char cdeck_ui_icon_png[] = {`n")

for ($i = 0; $i -lt $uiPngBytes.Length; $i += 16) {
    $end = [Math]::Min($i + 15, $uiPngBytes.Length - 1)
    $hexItems = for ($j = $i; $j -le $end; $j++) {
        "0x{0:X2}" -f $uiPngBytes[$j]
    }
    $line = "    " + ($hexItems -join ", ")
    if ($end -lt ($uiPngBytes.Length - 1)) {
        $line += ","
    }
    [void]$sb.Append($line + "`n")
}
[void]$sb.Append("};`n`n")
[void]$sb.Append("#endif`n")

$headerPath = Join-Path $srcDir "cdeck_icon_data.h"
$headerContent = $sb.ToString()
$existingHeader = if (Test-Path $headerPath) { [System.IO.File]::ReadAllText($headerPath) } else { "" }
if ($existingHeader -ne $headerContent) {
    [System.IO.File]::WriteAllText($headerPath, $headerContent, (New-Object System.Text.UTF8Encoding($false)))
}

Write-Host ("Generated multi-res ICO ({0} sizes, {1} bytes), 1024px PNG, and UI texture header." -f $entries.Count, $icoBytes.Length)
