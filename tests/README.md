# Alpha preprocessing resource cache regression

This fixture calls the actual runtime alpha preprocessing and submission cleanup
methods. It counts native `CreateComputeShader`/`CreateBuffer` calls and checks
resource identity, shader/buffer failure retries, cleanup and a replacement
device, and submissions requiring no alpha processing. Five dispatches are
queued before pixel readback, with changing viewport, alpha and sRGB constants.
Aligned viewports avoid testing unrelated shader boundary behavior.

Requirements: Windows, VS 2022 v143 C++ tools, a Windows SDK, and a hardware
D3D11 device supporting `ID3D11Device5`/`ID3D11DeviceContext4` and fences. No
headset or VR backend connection is required. The fixture reads existing runtime
settings but does not write them. Linking the existing runtime objects requires
its Vulkan, Detours and GameInput dependencies even though this test does not
exercise those APIs. Running the fixture also requires `vulkan-1.dll` on the
system or `PATH`. A missing dependency or unsupported device is a failure.

From the repository root, in PowerShell launched from the **VS x64 Native Tools
Command Prompt**, restore dependencies and rebuild the unsigned runtime core:

```powershell
git submodule update --init
nuget restore VirtualDesktop-OpenXR.sln
$repo = (Get-Location).Path.Replace('\', '/')
$objects = "$repo/bin/alpha-cache/obj"
New-Item -ItemType Directory -Force $objects | Out-Null
$commit = git rev-parse HEAD
Set-Content "$objects/commit.h" ('const char* RuntimeCommitHash = "' + $commit + '";')
msbuild virtualdesktop-openxr/virtualdesktop-openxr.vcxproj /t:Rebuild /m /p:Configuration=ReleaseBundle /p:Platform=x64 "/p:SolutionDir=$repo/" "/p:IntDir=$objects/" "/p:OutDir=$repo/bin/alpha-cache/" /p:PreBuildEventUseInBuild=false /p:PostBuildEventUseInBuild=false
if ($LASTEXITCODE) { throw 'Runtime build failed' }
powershell -NoProfile -ExecutionPolicy Bypass -File ./tests/Build-AlphaPreprocessCache.ps1 -RuntimeObjectDirectory $objects
```

Rebuild all objects with this checkout's exact current headers; do not use
objects from another fork or revision. Each mode runs in a separate process with
a 30-second limit and must return zero with a `PASS:` marker. Use `-Mode reuse`
to run only the five-dispatch counter/pixel check. Generated binaries and logs
stay under ignored `bin/`. Call counts measure removed resource-creation work;
this fixture does not measure game frame time or headset latency.
