# OpenXR runtime for Pimax headsets

This is a simple OpenXR runtime for Pimax devices.

DISCLAIMER: This software is distributed as-is, without any warranties or conditions of any kind. Use at your own risks.

## Setup

Download the latest version from the [Releases page](https://github.com/mbucchia/Pimax-OpenXR/releases). Find the installer program under **Assets**, file `Pimax-OpenXR.msi`.

For troubleshooting, the log file can be found at `%LocalAppData%\pimax-openxr.log`.

## Limitations

- Windows OS support only.
- Direct3D 11 support only.
- Limited support for quad layers (no alpha blending).
- No motion controller support.
- No depth reprojection support.
- No motion smoothing nor motion reprojection support.
- No hidden area mesh support.

## Known issues

- Compatible only with OpenXR Toolkit 1.1.0 (version 1.1.2 and above have serious menu issues due to the quad layer limitations above).

If you are having issues, please visit the [Issues page](https://github.com/mbucchia/Pimax-OpenXR/issues) to look at existing support requests or to file a new one.
