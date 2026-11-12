$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
New-Item -ItemType Directory -Force -Path "build" | Out-Null
$size = 256
$bitmap = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$graphics.Clear([System.Drawing.Color]::Transparent)
$tilePath = New-Object System.Drawing.Drawing2D.GraphicsPath
$tilePath.AddArc(10, 10, 72, 72, 180, 90); $tilePath.AddArc(174, 10, 72, 72, 270, 90); $tilePath.AddArc(174, 174, 72, 72, 0, 90); $tilePath.AddArc(10, 174, 72, 72, 90, 90); $tilePath.CloseFigure()
$tileBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush((New-Object System.Drawing.Rectangle(10,10,236,236)), [System.Drawing.Color]::FromArgb(255,24,27,34), [System.Drawing.Color]::FromArgb(255,7,9,12), 45)
$graphics.FillPath($tileBrush, $tilePath)
$rim = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(180,125,139,160), 2.2)
$graphics.DrawPath($rim, $tilePath)
$points = [System.Drawing.Point[]]@((New-Object System.Drawing.Point(183,65)),(New-Object System.Drawing.Point(107,65)),(New-Object System.Drawing.Point(62,110)),(New-Object System.Drawing.Point(62,146)),(New-Object System.Drawing.Point(107,191)),(New-Object System.Drawing.Point(183,191)),(New-Object System.Drawing.Point(183,159)),(New-Object System.Drawing.Point(120,159)),(New-Object System.Drawing.Point(94,133)),(New-Object System.Drawing.Point(94,123)),(New-Object System.Drawing.Point(120,97)),(New-Object System.Drawing.Point(183,97)))
$silverBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush((New-Object System.Drawing.Rectangle(62,65,121,126)), [System.Drawing.Color]::White, [System.Drawing.Color]::FromArgb(255,184,191,202), 35)
$graphics.FillPolygon($silverBrush, $points)
$limePoints = [System.Drawing.Point[]]@((New-Object System.Drawing.Point(143,191)),(New-Object System.Drawing.Point(187,191)),(New-Object System.Drawing.Point(218,159)),(New-Object System.Drawing.Point(174,159)))
$limeBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255,157,255,0))
$graphics.FillPolygon($limeBrush, $limePoints)
$stream = New-Object System.IO.MemoryStream
$bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
$png = $stream.ToArray()
$output = New-Object System.IO.MemoryStream
$writer = New-Object System.IO.BinaryWriter($output)
$writer.Write([UInt16]0); $writer.Write([UInt16]1); $writer.Write([UInt16]1)
$writer.Write([Byte]0); $writer.Write([Byte]0); $writer.Write([Byte]0); $writer.Write([Byte]0)
$writer.Write([UInt16]1); $writer.Write([UInt16]32); $writer.Write([UInt32]$png.Length); $writer.Write([UInt32]22)
$writer.Write($png); $writer.Flush()
[IO.File]::WriteAllBytes((Join-Path (Get-Location) "build/cdeck.ico"), $output.ToArray())
$writer.Dispose(); $output.Dispose(); $stream.Dispose(); $graphics.Dispose(); $bitmap.Dispose()
$tileBrush.Dispose(); $rim.Dispose(); $silverBrush.Dispose(); $limeBrush.Dispose(); $tilePath.Dispose()
Write-Host "Official C-Deck icon generated: build/cdeck.ico"
