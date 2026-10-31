param(
    [string]$V2Root = 'D:\project\05_workspace_v2\04_program\build'
)

$exe = Join-Path $V2Root 'NIUMA.exe'
if (-not (Test-Path -LiteralPath $exe)) {
    throw "v2 executable not found: $exe"
}

# The v2 binary is a Qt5/MinGW GUI application.  Inject the same runtime
# directories used by the v3 preview so launching the reference never falls
# back to the generic Windows 'system error' dialog.
$env:Path = "C:\msys64\mingw64\bin;C:\msys64\usr\bin;$env:Path"
Remove-Item Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue

Get-Process -Name NIUMA -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Process -FilePath $exe -WorkingDirectory $V2Root
