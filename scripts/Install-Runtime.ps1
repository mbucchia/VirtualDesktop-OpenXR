if (Test-Path "pimax-openxr.json") {
	$RegistryPath = "HKLM:\Software\Khronos\OpenXR\1"
	$JsonPath = Join-Path "$PSScriptRoot" "pimax-openxr.json"
} elseif (Test-Path "pimax-openxr-32.json") {
	$RegistryPath = "HKLM:\Software\WOW6432Node\Khronos\OpenXR\1"
	$JsonPath = Join-Path "$PSScriptRoot" "pimax-openxr-32.json"
} else {
	Exit
}
Start-Process -FilePath powershell.exe -Verb RunAs -Wait -ArgumentList @"
	& {
		If (-not (Test-Path $RegistryPath)) {
			New-Item -Path $RegistryPath -Force | Out-Null
		}
		New-ItemProperty -Path $RegistryPath -Name ActiveRuntime -PropertyType String -Value '$jsonPath' -Force | Out-Null
	}
"@
