# OpenXR runtime for Pimax headsets

This is a minimal OpenXR runtime for Pimax devices.

DISCLAIMER: This software is distributed as-is, without any warranties or conditions of any kind. Use at your own risks.

## Setup

Download the latest version from the [Releases page](https://github.com/mbucchia/Pimax-OpenXR/releases). Find the installer program under **Assets**, file `Pimax-OpenXR.msi`.

For troubleshooting, the log file can be found at `%LocalAppData%\pimax-openxr.log`.

## Limitations

- Windows OS support only.
- Direct3D support only.
  - Direct3D 11 is supported out-of-the-box.
  - Direct3D 12 is supported via [OpenXR-D3D12on11](https://github.com/mbucchia/OpenXR-D3D12on11).
- Limited support for quad layers (no alpha blending).
- No motion controller support.
- No depth reprojection support.
- No hidden area mesh support.

So far, it has only been tested with Microsoft Flight Simulator 2020 and DCS (through the use of [OpenComposite](https://gitlab.com/znixian/OpenOVR/-/tree/openxr)), and with a Pimax 8KX.

## Known issues

- Not yet compatible with the OpenXR Toolkit.

If you are having issues, please visit the [Issues page](https://github.com/mbucchia/Pimax-OpenXR/issues) to look at existing support requests or to file a new one.
