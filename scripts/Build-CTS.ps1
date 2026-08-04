$BuildType = "RelWithDebInfo"

cmake -G "Visual Studio 17 2022" -A "x64" -B $PSScriptRoot\..\bin\x64\CTS -S $PSScriptRoot\..\external\OpenXR-CTS `
    -DDYNAMIC_LOADER=ON `
    -DBUILD_LOADER=ON `
    -DBUILD_API_LAYERS=OFF `
    -DBUILD_TESTS=OFF `
    -DBUILD_CONFORMANCE_TESTS=ON
if (-Not $?)
{
    throw "CMake generate failed: $LastExitCode"
}

cmake --build $PSScriptRoot\..\bin\x64\CTS --config $BuildType --parallel 8
if (-Not $?)
{
    throw "CMake build failed: $LastExitCode"
}

Write-Host "All output in: $PSScriptRoot\..\bin\x64\CTS\src\conformance\conformance_cli\$BuildType"
