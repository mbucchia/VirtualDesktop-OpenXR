param(
    [Parameter(Mandatory = $true)][string]$RuntimeObjectDirectory,
    [ValidateSet('reuse', 'shader-retry', 'buffer-retry', 'cleanup', 'partial-cleanup', 'no-work', 'all')]
    [string]$Mode = 'all'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$objectsPath = (Resolve-Path -LiteralPath $RuntimeObjectDirectory).Path
foreach ($name in @('frame.obj', 'instance.obj', 'd3d11_native.obj')) {
    if (-not (Test-Path -LiteralPath (Join-Path $objectsPath $name))) {
        throw 'Supply fresh x64 ReleaseBundle objects from this checkout, built with its current headers.'
    }
}
# Run in the VS x64 native tools environment, after restoring the project's dependencies.
$null = Get-Command cl.exe, link.exe -ErrorAction Stop
$output = Join-Path $repo ('bin/alpha-cache-test-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$includes = @(
    'virtualdesktop-openxr', 'external/OpenXR-SDK/include', 'external/OpenXR-SDK/src/common',
    'external/OpenXR-MixedReality/Shared', 'external/OpenXR-MixedReality/Shared/XrUtility',
    'external/OpenXR-MixedReality/Shared/SampleShared', 'external/LibOVR/include',
    'external/LibOVR/include/Extras', 'external/LibOVR/Shim', 'external/Vulkan-SDK/include',
    'external/OpenGL', 'external/fmt/include', 'external/openvr/headers',
    'external/openvr/samples/drivers/drivers/handskeletonsimulation/src',
    'external/openvr/samples/drivers/utils/vrmath', 'external/cJSON',
    'packages/Detours.4.0.1/lib/native/include',
    'packages/Microsoft.Windows.ImplementationLibrary.1.0.220201.1/include',
    'packages/Microsoft.GameInput.2.2.26100.6114/native/include'
)
$options = @('/nologo', '/std:c++17', '/EHsc', '/W3', '/MT', '/O2', '/Gy', '/c',
    '/DRUNTIME_NAMESPACE=virtualdesktop_openxr', '/DNDEBUG', '/DUNICODE', '/D_UNICODE',
    '/D_CRT_SECURE_NO_WARNINGS', '/DUSING_GAMEINPUT', "/I$objectsPath")
foreach ($include in $includes) { $options += '/I' + (Join-Path $repo $include) }
$testObject = Join-Path $output 'alpha_preprocess_cache.obj'
& cl.exe @options (Join-Path $PSScriptRoot 'alpha_preprocess_cache.cpp') "/Fo$testObject"
if ($LASTEXITCODE) { exit $LASTEXITCODE }
$objects = @(Get-ChildItem -LiteralPath $objectsPath -Filter '*.obj' | ForEach-Object FullName)
$libraries = @('dxgi.lib', 'dxguid.lib', 'd3d11.lib', 'd3d12.lib', 'opengl32.lib', 'ntdll.lib',
    'kernel32.lib', 'user32.lib', 'gdi32.lib', 'winspool.lib', 'comdlg32.lib', 'advapi32.lib',
    'shell32.lib', 'ole32.lib', 'oleaut32.lib', 'uuid.lib', 'odbc32.lib', 'odbccp32.lib',
    (Join-Path $repo 'packages/Detours.4.0.1/lib/native/libs/x64/detours.lib'),
    (Join-Path $repo 'packages/Microsoft.GameInput.2.2.26100.6114/native/lib/x64/GameInput.lib'),
    (Join-Path $repo 'external/Vulkan-SDK/lib/vulkan-1.lib'))
$executable = Join-Path $output 'alpha_preprocess_cache.exe'
& link.exe /nologo /LTCG /SUBSYSTEM:CONSOLE "/OUT:$executable" $testObject @objects @libraries
if ($LASTEXITCODE) { exit $LASTEXITCODE }
Write-Output "Fixture: $executable"
$modes = if ($Mode -eq 'all') { @('reuse', 'shader-retry', 'buffer-retry', 'cleanup', 'partial-cleanup', 'no-work') } else { @($Mode) }
foreach ($case in $modes) {
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $executable
    $info.Arguments = $case
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $process = [Diagnostics.Process]::Start($info)
    try {
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit(30000)) { $process.Kill(); $process.WaitForExit(); throw "Timed out: $case" }
        $text = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
        [IO.File]::WriteAllText((Join-Path $output ($case + '.log')), $text)
        Write-Output $text
        if ($process.ExitCode -ne 0 -or $text -notmatch '(?m)^PASS:' -or $text -match '(?m)^FAIL:') {
            throw "Regression failed: $case (exit $($process.ExitCode))"
        }
    } finally { $process.Dispose() }
}
Write-Output "PASS: $($modes.Count) alpha resource cache cases"
