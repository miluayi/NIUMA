param(
    [Parameter(Mandatory=$true)][string]$Application,
    [Parameter(Mandatory=$true)][string]$RuntimeDirectory,
    [Parameter(Mandatory=$true)][string]$PluginsDirectory,
    [Parameter(Mandatory=$true)][string]$Objdump,
    [Parameter(Mandatory=$true)][string]$Report
)
$ErrorActionPreference='Stop'
# Audit only imports, not strings naming optional dynamically resolved APIs.
# This catches loader failures; it cannot replace execution on Windows 7 SP1.
$forbidden=@(
    'GetSystemTimePreciseAsFileTime','GetSystemTimeAdjustmentPrecise',
    'GetDpiForWindow','GetDpiForSystem','GetDpiForMonitor','GetSystemMetricsForDpi',
    'AdjustWindowRectExForDpi','SetProcessDpiAwareness','SetProcessDpiAwarenessContext',
    'SetThreadDpiAwarenessContext','GetThreadDpiAwarenessContext',
    'SetThreadDescription','GetThreadDescription','WaitOnAddress',
    'WakeByAddressSingle','WakeByAddressAll','GetOverlappedResultEx','CreateFile2',
    'GetTempPath2W','GetTempPath2A','GetSystemCpuSetInformation',
    'SetThreadSelectedCpuSets','GetProcessInformation','SetProcessInformation',
    'GetThreadInformation','SetThreadInformation','GetProcessMitigationPolicy',
    'SetProcessMitigationPolicy','GetCurrentPackageFullName','GetCurrentPackageFamilyName',
    'InitializeSynchronizationBarrier','DeleteSynchronizationBarrier',
    'EnterSynchronizationBarrier','DiscardVirtualMemory','OfferVirtualMemory',
    'ReclaimVirtualMemory','GetPointerInfo','GetPointerType','EnableMouseInPointer',
    'CreateDXGIFactory2','DXGIGetDebugInterface1','DCompositionCreateDevice',
    'DCompositionCreateDevice2','DCompositionCreateDevice3'
)
$system=@(
    'ADVAPI32.dll','AUTHZ.dll','AVICAP32.dll','BCRYPT.dll','CRYPT32.dll','CRYPTBASE.dll',
    'D3D9.dll','D3D11.dll','DBGHELP.dll','DNSAPI.dll','DWMAPI.dll','DWRITE.dll','DXGI.dll',
    'GDI32.dll','IMM32.dll','IPHLPAPI.dll','KERNEL32.dll','MPR.dll','MSIMG32.dll','MSVCRT.dll',
    'NETAPI32.dll','NTDLL.dll','ODBC32.dll','OLE32.dll','OLEACC.dll','OLEAUT32.dll','OPENGL32.dll',
    'POWRPROF.dll','PROPSYS.dll','PSAPI.dll','RPCRT4.dll','SECUR32.dll','SETUPAPI.dll',
    'SHELL32.dll','SHLWAPI.dll','USER32.dll','USERENV.dll','USP10.dll','UXTHEME.dll','UUID.dll','VERSION.dll',
    'WINHTTP.dll','WININET.dll','WINMM.dll','WINSPOOL.DRV','WINTRUST.dll','WLDAP32.dll',
    'WS2_32.dll','WTSAPI32.dll'
)
$pending=New-Object 'System.Collections.Generic.Queue[string]'
$pending.Enqueue((Resolve-Path -LiteralPath $Application).Path)
$plugins=@('platforms/qwindows.dll','iconengines/qsvgicon.dll',
    'styles/qwindowsvistastyle.dll','sqldrivers/qsqlite.dll','sqldrivers/qsqlodbc.dll')
$plugins+=@(Get-ChildItem -LiteralPath (Join-Path $PluginsDirectory 'imageformats') -Filter '*.dll' |
    ForEach-Object { 'imageformats/'+$_.Name })
foreach ($plugin in $plugins) {
    $pending.Enqueue((Resolve-Path -LiteralPath (Join-Path $PluginsDirectory $plugin)).Path)
}
$seen=@{}
$binaries=@()
$failures=@()
while ($pending.Count) {
    $path=$pending.Dequeue()
    if ($seen.ContainsKey($path)) { continue }
    $seen[$path]=$true
    $dump=@(& $Objdump -p $path)
    if ($LASTEXITCODE) { throw "Cannot inspect $path" }
    $dll=''
    $imports=@()
    foreach ($line in $dump) {
        if ($line -match 'DLL Name:\s+(\S+)') {
            $dll=$Matches[1]
            if ($dll -match '^(combase|shcore|dcomp|d3d12)\.dll$|^api-ms-win-core-(winrt|path)-') {
                $failures+="$path -> $dll"
            }
            if ($dll -notin $system) {
                $dependency=Join-Path $RuntimeDirectory $dll
                if (-not (Test-Path -LiteralPath $dependency)) {
                    $dependency=Join-Path (Split-Path -Parent $path) $dll
                }
                if (Test-Path -LiteralPath $dependency) {
                    $pending.Enqueue((Resolve-Path -LiteralPath $dependency).Path)
                } else { $failures+="$path -> unresolved $dll" }
            }
        } elseif ($line -match '^\s*[0-9a-fA-F]+\s+(?:<none>\s+)?[0-9a-fA-F]+\s+(\S+)\s*$') {
            $name=$Matches[1]
            $imports+=@{Dll=$dll;Function=$name}
            if ($name -in $forbidden) { $failures+="$path -> $dll!$name" }
        }
    }
    $major=0
    $minor=0
    foreach ($line in $dump) {
        if ($line -match '^MajorSubsystemVersion\s+(\d+)') { $major=[int]$Matches[1] }
        if ($line -match '^MinorSubsystemVersion\s+(\d+)') { $minor=[int]$Matches[1] }
    }
    if ($major -gt 6 -or ($major -eq 6 -and $minor -gt 1)) {
        $failures+="$path -> subsystem $major.$minor exceeds Windows 7"
    }
    $binaries+=@{File=$path;Subsystem="$major.$minor";Imports=$imports}
}
$reportPath=[IO.Path]::GetFullPath($Report)
[IO.Directory]::CreateDirectory((Split-Path -Parent $reportPath)) | Out-Null
@{Scope='Static imports and PE subsystem; not a Windows 7 execution test';
  BinaryCount=$binaries.Count;Failures=$failures;Binaries=$binaries} |
    ConvertTo-Json -Depth 7 | Set-Content -LiteralPath $reportPath -Encoding UTF8
if ($failures.Count) { $failures | Write-Output; exit 1 }
Write-Output "Windows 7 import audit passed: $($binaries.Count) binaries, no known newer mandatory APIs"
