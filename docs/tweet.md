# nitro-control tweets — pick one

## Primary (dev-story angle)

Shipped a native Linux fan+RGB controller for my Acer Nitro AN515-58 in C++/Qt6.

2,600 lines. One 250 KB binary. No Electron, no Python, no 88 MB RSS.

- 4 fan modes + manual per-fan duty (real EC writes via nbfc)
- thermal guard: ≥88°C emergency rollback, anti-flap hysteresis
- live keyboard RGB preview, survives reboot & suspend
- click-path tested by its own UI selftest (17 assertions, exit code = pass/fail)

Layered core/ui/app, zero warnings.

MIT: https://github.com/Kmassidik/acer-nitro-control

#Linux #KDE #Qt #Cplusplus #FOSS

## Short emotional one

Rewrote my laptop's fan/RGB control from Python (88 MB) to C++/Qt6: 250 KB binary, instant startup, full EC fan control + RGB that survives reboot. 2.6k lines, self-tested UI. MIT: https://github.com/Kmassidik/acer-nitro-control #Linux

## Security-lesson angle

PSA: I accidentally shipped my sudo password inside a fan-control tool I wrote — hardcoded in a helper script, living in git history.

Fixed properly: passwordless root via 3 exact-argv NOPASSWD lines pointing at a whitelist helper that fails closed. Purged history with fast-export/sed/fast-import, force-pushed, rotated.

Lesson: never `echo <pw> | sudo -S`. It ends up in every commit forever.

Lesson lesson: write a UI selftest so "it works" is an exit code, not a feeling.

https://github.com/Kmassidik/acer-nitro-control
