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
$points = [System.Drawing.Point[]]@([System.Drawing.Point]::new(183,65),[System.Drawing.Point]::new(107,65),[System.Drawing.Point]::new(62,110),[System.Drawing.Point]::new(62,146),[System.Drawing.Point]::new(107,191),[System.Drawing.Point]::new(183,191),[System.Drawing.Point]::new(183,159),[System.Drawing.Point]::new(120,159),[System.Drawing.Point]::new(94,133),[System.Drawing.Point]::new(94,123),[System.Drawing.Point]::new(120,97),[System.Drawing.Point]::new(183,97))
$silverBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush((New-Object System.Drawing.Rectangle(62,65,121,126)), [System.Drawing.Color]::White, [System.Drawing.Color]::FromArgb(255,184,191,202), 35)
$graphics.FillPolygon($silverBrush, $points)
$limePoints = [System.Drawing.Point[]]@([System.Drawing.Point]::new(143,191),[System.Drawing.Point]::new(187,191),[System.Drawing.Point]::new(218,159),[System.Drawing.Point]::new(174,159))
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
