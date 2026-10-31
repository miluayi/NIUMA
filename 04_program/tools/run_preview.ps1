param(
    [switch]$Shoot,
    [switch]$SchemeB,
    [string]$CaptureDir
)

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$previewExe = Join-Path $repoRoot '04_program\build\ui-preview-debug\04_program\ui_preview\NiumaV2SourcePreview.exe'

if (-not (Test-Path -LiteralPath $previewExe)) {
    throw "Preview executable not found: $previewExe. Build the ui-preview-debug preset first."
}

$env:NIUMA_PREVIEW_DATA_DIR = Join-Path $repoRoot '06_test\data\ui-preview-debug\v2-preview-data-b'

# Qt is installed through the MinGW toolchain in the development environment.
# Keep the runtime path explicit so launching from Explorer or Codex has the
# same result as launching from a configured build shell.
$env:Path = "C:\msys64\mingw64\bin;C:\msys64\usr\bin;$env:Path"
Remove-Item Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue

$arguments = @()
# -SchemeB remains accepted so existing shortcuts keep working; B is the only preview.
if ($Shoot) {
    if ([string]::IsNullOrWhiteSpace($CaptureDir)) {
        $CaptureDir = Join-Path $repoRoot '06_test\evidence\ui-preview-debug'
    }
    $arguments += @('--shoot', '--capture-dir', $CaptureDir)
}

Start-Process -FilePath $previewExe -WorkingDirectory $repoRoot -ArgumentList $arguments
