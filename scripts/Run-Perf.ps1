param (
    [string]$ProjectDir = "$PSScriptRoot/..",
    [string[]]$Revisions=@("HEAD"),
    [string]$DevEnv = "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.com",
    [string]$PerfTestDir = "D:\XR\XrPerfTest\Output"
)

$Solution = Resolve-Path "${ProjectDir}\Pimax-OpenXR.sln"
$RuntimeJson = Resolve-Path "${ProjectDir}\bin\x64\Release\Pimax-openxr.json"
$PerfTest = "${PerfTestDir}\XrPerfTest.exe"

$ErrorActionPreference = "Stop"
function ThrowOnNativeFailure {
    if ($LastExitCode -ne 0) {
        throw 'Error executing command'
    }
}

if (-not $(Test-Path -Path $PerfTest -PathType leaf)) {
    Write-Host -ForegroundColor Red "Could not find ${PerfTest}. Please check that the project is built."
    exit 1
}

foreach ($revision in $Revisions) {
    git checkout $ProjectDir
    git checkout $revision
    ThrowOnNativeFailure
    git submodule update
    ThrowOnNativeFailure

    git rev-parse

    & $DevEnv $Solution /Rebuild "Release|x64" /Project "pimax-openxr"
    ThrowOnNativeFailure

    Remove-Item "${PerfTestDir}\perf.csv" -ErrorAction SilentlyContinue
    Remove-Item "${PerfTestDir}\ScreenCaptureAtEnd.png" -ErrorAction SilentlyContinue

    $env:XR_RUNTIME_JSON=$RuntimeJson
    Push-Location $PerfTestDir
    & $PerfTest
    ThrowOnNativeFailure
    Pop-Location

    while (-not $(Test-Path -Path "${PerfTestDir}\ScreenCaptureAtEnd.png" -PathType leaf)) {
        Sleep 1
    }

    Copy-Item "${PerfTestDir}\perf.csv" "perf-${revision}.csv"
    Copy-Item "${PerfTestDir}\ScreenCaptureAtEnd.png" "ScreenCaptureAtEnd-${revision}.png"
}
