# facer DKMS patch: `gaming_fan_mode` — real fan control on AN515-58

## Why

The AN515-58 "Jimny_ADH" firmware (BIOS V1.11) **ignores the legacy EC fan
registers** `nitro-fan` used (unlock 0x03=0x51, manual bits 0x34/0x33,
duty 0x37/0x3A). Writing 100% duty there succeeds byte-wise but the EC fan
engine never acts on it (verified live: duty byte sticks, RPM unchanged).

The firmware's real fan engine is the **Predator/Gaming WMI fan-behavior
call** (method 14 = `ACER_WMID_SET_GAMING_FAN_BEHAVIOR`, what the physical
turbo button does). `facer` (predator_v4) calls it internally but exposes
no userspace interface.

## The patch (applied to `/usr/src/facer-0.20241016-1bP/src/facer.c`)

Three edits:

1. **Extra sysfs attribute** after `WMID_gaming_set_fan_mode()`:
   `gaming_fan_mode` (RW) — `1` = auto (firmware curve), `2` = turbo
   (aggressive curve, high RPM floor). Verified live:
   auto ≈ 2100–2600 RPM idle, turbo ≈ 6000–7700 RPM.
2. **Registration** in `acer_wmi_hwmon_init()` via
   `device_create_file(&acer_platform_device->dev, &dev_attr_gaming_fan_mode)`.
3. **Capability fix**: when loaded with `predator_v4=1` as a module
   parameter, `find_quirks()` short-circuits to `quirk_acer_predator_v4`,
   which had `.turbo = 0` — so `ACER_CAP_TURBO_FAN` was never granted and
   every write returned `EOPNOTSUPP`. The quirk now carries
   `.turbo = 1, .cpu_fans = 1, .gpu_fans = 1`.

## Deployment notes

- Plain in-tree `make` in `/usr/src/facer-<ver>` works; the DKMS wrapper
  build may fail silently — verify by grepping the built `.ko` for the
  `gaming_fan_mode` symbol before trusting it.
- Compress with a **1 MiB dictionary** (`xz --lzma2=dict=1MiB`) at most —
  the kernel xz decompressor rejects the default `-9e` stream
  (`decompression failed, status 6`). Uncompressed `.ko` in
  `/usr/lib/modules/<kver>/extra/` also works.
- Then `depmod <kver>` and reload with `predator_v4=1`.
- `/etc/modprobe.d/facer.conf` must carry `options facer predator_v4=1`
  (fan RPM reads + this attribute all depend on it).

## App wiring

`nitro-priv fanmode auto|turbo` (whitelisted in sudoers) writes the attr;
`turbo-lvl 3|4` engages turbo fan behavior, `1|2` keep the curve.
