# ZenOverlay local validation

## November 26, 2026 — Elements Divided

The user reported that the headset test passed and separately confirmed that
OBS worked. This was an unsigned x64 runtime test in Elements Divided, not an
anti-cheat test. No recording was required.

- Runtime code revision: `841680e5c7e83da1f868d1bf22c39c3aed078c27`.
- Runtime DLL SHA-256: `359355A1E0F3BF83870F3B5391A0333D8E1C178C91E58DB6A9206DF574DE311B`.
- Companion: signed RC4 desktop controller/capture, supplied separately.
- Runtime log: Elements.exe, D3D11, Virtual Desktop Streamer 1.34.25, Quest 3.
- Session recorded approximately 07:46:30–07:49:00 America/Chicago.

The runtime log identifies the exact code revision and records texture openings
at 762x472, 1910x1018, then 762x472; head-follow and left-wrist settings; and an
A+B+X+Y hidden/shown pair. No ZenOverlay error appears in that runtime log.
The Unity log records orderly OpenXR STOPPING, IDLE and EXITING transitions and
shutdown. Its OVRPlugin/hand-tracking warnings also occur in the preceding game
log; they are not evidence of a new ZenOverlay failure.

OBS stability is user-observed. The available OBS log ended before this gameplay
session, so it does not independently corroborate in-session frame health. The
game log confirms the OBS Mirror API layer loaded. The runtime-appended overlay
is not expected in OBS output because the private recording bridge is excluded.

This is a headset smoke-test pass, not independent proof of every checklist item.
Forced producer/controller crashes, right-wrist behavior, profile round trips,
and all other placement combinations were not individually evidenced in the
saved log. Earlier recovery tests and the isolated checks below remain separate
supporting evidence. No new Skyrim/DLSS, Win32 headset, multi-GPU, performance,
or anti-cheat compatibility result is claimed.

## November 25, 2026 — non-VR checks

Both x64 and Win32 ReleaseBundle runtime builds passed with zero warnings/errors.
The separate desktop source rebuilt successfully. Eighty-one checks passed:
21 process/event lifecycle, 23 settings/shortcut, 20 desktop selection and 17
native capture-selection assertions. Lifecycle/settings fixtures use extracted
production helpers with isolated object names; OVR button input is mocked.

The runtime patches were applied to a fresh temporary index at upstream
`6da20fe478e331c05672243fd82a9a1500cd39f5` and reproduced the candidate tree exactly.
The desktop test ZIP was verified against source/binary hashes and existing valid
signatures. These checks do not replace physical headset observations.
