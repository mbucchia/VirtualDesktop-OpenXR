$BuildType = "RelWithDebInfo"

cmake -G "Visual Studio 16 2019" -A "x64" -B $PSScriptRoot\..\bin\x64\CTS -S $PSScriptRoot\..\external\OpenXR-CTS `
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

# Cleanup past prebuilt.
If (Test-Path $PSScriptRoot\..\bin\x64\CTS\output)
{
    Remove-Item $PSScriptRoot\..\bin\x64\CTS\output -Recurse -Force
}

New-Item $PSScriptRoot\..\bin\x64\CTS\output -ItemType Directory | Out-Null
Copy-Item $PSScriptRoot\..\bin\x64\CTS\src\conformance\conformance_cli\$BuildType\conformance_cli.exe $PSScriptRoot\..\bin\x64\CTS\output -Force
Copy-Item $PSScriptRoot\..\bin\x64\CTS\src\conformance\conformance_cli\$BuildType\conformance_cli.pdb $PSScriptRoot\..\bin\x64\CTS\output -Force
Copy-Item $PSScriptRoot\..\bin\x64\CTS\src\conformance\conformance_test\$BuildType\conformance_test.dll $PSScriptRoot\..\bin\x64\CTS\output -Force
Copy-Item $PSScriptRoot\..\bin\x64\CTS\src\conformance\conformance_test\$BuildType\conformance_test.pdb $PSScriptRoot\..\bin\x64\CTS\output -Force
Copy-Item $PSScriptRoot\..\bin\x64\CTS\src\conformance\conformance_cli\*.png $PSScriptRoot\..\bin\x64\CTS\output -Force
Copy-Item $PSScriptRoot\..\bin\x64\CTS\src\conformance\conformance_cli\*.otf $PSScriptRoot\..\bin\x64\CTS\output -Force
Copy-Item $PSScriptRoot\..\bin\x64\CTS\src\conformance\conformance_test\$BuildType\openxr_loader.dll $PSScriptRoot\..\bin\x64\CTS\output -Force
Copy-Item $PSScriptRoot\..\bin\x64\CTS\src\conformance\conformance_layer\$BuildType\XrApiLayer_runtime_conformance.dll $PSScriptRoot\..\bin\x64\CTS\output -Force
Copy-Item $PSScriptRoot\..\bin\x64\CTS\src\conformance\conformance_layer\$BuildType\XrApiLayer_runtime_conformance.pdb $PSScriptRoot\..\bin\x64\CTS\output -Force
Copy-Item $PSScriptRoot\..\bin\x64\CTS\src\conformance\conformance_layer\XrApiLayer_runtime_conformance.json $PSScriptRoot\..\bin\x64\CTS\output -Force

Write-Host "All output in: $PSScriptRoot\..\bin\x64\CTS\output"
