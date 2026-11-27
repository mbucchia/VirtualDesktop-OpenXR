# External desktop overlays

ZenOverlay is an external Windows desktop capture producer and controller. This
integration consumes one completed D3D11 texture and appends an OVR quad after the
application layers, before the existing frame submission path. It does not add
desktop capture, a GUI, an installer, or a phone application to VDXR.

The controller starts and manages its capture process. VDXR needs no additional
OpenXR API layer for this overlay. The companion and its test instructions are
provided separately from this runtime contribution.

## Activation and lifetime

The runtime creates and retains the manual-reset ready event below. Idle checks
use a zero-time wait; creation is retried after failure. A signaled event opts in
to texture discovery, but is not sufficient by itself: the runtime also opens a
live producer process handle from the status mapping. The process handle remains
bound to that process even if its PID is later reused.

Resetting readiness or detecting producer exit releases the shared texture,
mutex, private snapshot and OVR swapchain. The event survives reconnects. A new
connection opens the producer resources again. Session teardown releases graphics
resources before the submission device is destroyed. Settings/event handles can
remain cached for the runtime object's lifetime.

An alive but stalled producer can leave its last completed image visible. There
is no heartbeat timeout or forced process termination in this contribution.

## Existing IPC contract

All names use the Windows `Local` namespace and default object security. These
are cooperative local-session channels, not authenticated or hostile-process
isolation. Runtime and producer must run in the same Windows session with
compatible access rights. One producer and one consuming runtime are supported;
simultaneous VR consumers are not supported by this single-consumer handshake.

| Object | Name | Purpose |
| --- | --- | --- |
| Manual-reset event | `Local\ZenOverlayCaptureReady_v1` | Capture ready for connection |
| Named shared D3D11 texture | `Local\ZenOverlayCaptureTexture_v1` | BGRA8 UNORM, single-sample desktop image |
| Read-only status mapping | `Local\ZenOverlayCaptureStatus_v1` | First 16 bytes: four uint32 values (magic `0x5A454E31`, version 2, status, PID) |
| Settings mapping | `Local\ZenOverlayRuntimeSettings_v3` | Complete 64-byte placement snapshot |
| Mutex | `Local\ZenOverlayRuntimeSettingsMutex_v3` | Protects settings publication/read |
| Manual-reset event | `Local\ZenOverlayVisible_v1` | Signaled means visible |
| Manual-reset event | `Local\ZenOverlayFaceShortcutEnabled_v1` | Enables the A+B+X+Y visibility shortcut |

The producer acquires keyed-mutex key 0, writes a complete frame (including the
cursor), then releases key 1. VDXR acquires key 1 with zero timeout, copies into a
private GPU snapshot and releases key 0. Only `S_OK` grants ownership: timeout
is not success. On timeout the last committed image is reused; before the first
completed image no quad is submitted. Abandoned/failed ownership causes resource
cleanup and reconnect. No GPU-to-CPU pixel transfer is added by the runtime.

The producer resets readiness during resource replacement and retries named
texture creation until the consumer releases its old resources. It must publish
the status header before signaling readiness. A producer must never treat a
consumer-held shared texture as proof that the producer itself is still alive.

Settings use little-endian fields at these offsets:

| Offset | Type | Meaning |
| --- | --- | --- |
| 0, 4, 8, 12 | uint32 x4 | Magic `0x5A525331`, version 3, bytes 64, flags |
| 16 | uint64 | Increasing nonzero revision, including across controller restarts |
| 24 | int32 | 0 left wrist, 1 fixed LOCAL, 2 head locked, 3 player yaw, 4 right wrist |
| 28–56 | float x8 | X, Y, Z (metres), pitch, yaw, roll (degrees), width, height (metres) |
| 60 | uint32 | Preserve captured aspect ratio: 0 or 1 |

Flags: bit 0 enables gaze visibility; bit 1 enables green-screen keying. Unknown
flags/versions, stale revisions, non-finite and out-of-range values are ignored.
The reader never waits for the settings mutex. An abandoned mutex is released
without consuming that snapshot. The controller must continue increasing the
revision if the mapping survives its restart. Capture readiness is retained;
other absent settings objects are discovered with a bounded 500 ms probe cadence.

## Presentation and controls

The compositor-facing swapchain is sRGB, with a typeless backing resource for
copies from BGRA8 UNORM desktop capture. Green-screen processing uses compiled
shader bytecode and linear premultiplied output. When submission uses the application's D3D11 device, it saves context state through the existing runtime mechanism; the `xrEndFrame` scope guard restores it.
Toggling keying can reprocess the private snapshot even for a static window.

The runtime supports fixed LOCAL, head-locked, player-yaw and both wrist poses.
Placement/presets/profiles are authored by the external controller. Gaze decisions
use hysteresis; wrist pose smoothing and aspect ratio behavior are retained from
the tested implementation. Head-locked placement does not apply gaze gating.
The optional A+B+X+Y shortcut requires a focused session, both Touch controllers,
and full button release before rearming. Hidden overlays still consume completed
frames so the producer continues to advance.

Application layers retain priority at `ovrMaxLayerCount`; the optional quad is
omitted when no compositor layer slot remains. While the layer budget is full,
overlay consumption/lifecycle polling is also deferred until a slot is available.
The producer must tolerate a consumer that temporarily does not acquire key 1.

## Scope and validation limits

The first contribution intentionally excludes multiple overlays, the private OBS
texture-export bridge, the former fallback API layer, and phone features. OpenXR
capture layers do not automatically see a quad appended inside the runtime.
This does not change their normal application capture path.

Earlier local builds exercised placement, visibility, transparency, stop/restart
and process recovery in VR. Those observations are supporting evidence, not a
fresh headset test of this extracted patch. Build and isolated-check results for
the exact proposed revision belong in the PR description. No universal game,
anti-cheat, multi-GPU, or asynchronous-submission stress certification is claimed.
