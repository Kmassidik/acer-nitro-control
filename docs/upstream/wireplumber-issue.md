# WirePlumber issue draft

**Title:** `sof-hda-dsp (UCM): jack plug/unplug across HiFi profiles — profile race drifts card to pro-audio and effect sinks lose their target`

**Repo:** https://gitlab.freedesktop.org/pipewire/wire-plumber

---

## Environment

- Fedora 44, WirePlumber 0.5.x (PipeWire 1.6.8)
- Laptop: Acer Nitro AN515-58 (Alder Lake PCH-P, `sof-hda-dsp`, card name `alsa_card.pci-0000_00_1f.3-platform-skl_hda_dsp_generic`)
- ALSA UCM defines two **mutually exclusive** HiFi profiles: `HiFi (HDMI1, HDMI2, HDMI3, Headphones, Mic1, Mic2)` and `HiFi (HDMI1, HDMI2, HDMI3, Mic1, Mic2, Speaker)`

## Symptoms

1. **Headset plugged in after boot never appears.** PipeWire picks the Speaker profile at device init; the Headphones port is not defined inside that profile, so no port-availability event ever fires when the jack is plugged. The Headphones sink is unobtainable without an external profile switch.
2. **On profile switch, the card drifts onto the raw `pro-audio` profile.** After an external actor (`pactl set-card-profile`) switches Speaker↔Headphones following jack events, WirePlumber's restore policy re-applies the stale stored profile and the card ends up with `Pro`-numbered sinks (`…Controller Pro`, `Pro 3`…"Pro 5`) which output silence. Observed twice ~2026-10-31 (unplug at 07:08 local).
3. **Effect chains are not retargeted** when their bound node disappears with the profile — a running EasyEffects `easyeffects_sink` stayed latched to the vanished Headphones node, producing system-wide silence from the default sink.

## Reproduction

```bash
# 0) initial state: Speaker profile, no headphones plugged
wpctl status                                   # active profile: HiFi (…Speaker)
amixer -c1 contents | grep -A2 "name='Headphone Jack'"   # values=off

# 1) plug a headset → jack state flips in-kernel
amixer -c1 contents | grep -A2 "name='Headphone Jack'"   # AFTER: values=on
# but NO Headphones sink appears; wpctl status unchanged.

# 2) external profile switch fixes the sink …
pactl set-card-profile alsa_card.pci-0000_00_1f.3-platform-skl_hda_dsp_generic \
  "HiFi (HDMI1, HDMI2, HDMI3, Headphones, Mic1, Mic2)"

# 3) …but on the switch back, the card can end on pro-audio:
pactl set-card-profile alsa_card.pci-0000_00_1f.3-platform-skl_hda_dsp_generic \
  "HiFi (HDMI1, HDMI2, HDMI3, Mic1, Mic2, Speaker)"
wpctl status                       # sinks become "…Controller Pro / Pro 3 …" (silent)
```

A userspace watcher (poll `Headphone Jack`, set profile, re-assert after 2 s to win
the race with restore-policy, restart the effects service) works around all three,
which indicates each fix belongs in WirePlumber's audio device/profile policy:

- react to jack events even when the port crosses the active profile's boundary
  (or watch UCM device availability),
- make `state-profile` restore verify observed state before re-applying (it
  currently re-applies a profile that is no longer the one the jack suggests),
- keep external user profile switches sticky (the re-assert hack is a race we
  had to add specifically because restore-policy overrode our switch).

## Journal evidence

```
Oct 31 07:08:55 fedora hdajack-profile[35115]: headset unplugged → Speaker profile
# afterwards: wpctl sinks = "…Controller Pro / Pro 3..5" instead of Speaker
Nov 01 07:12:31 fedora hdajack-profile[36746]: jack off → profile HiFi (…Speaker)
# second asserted write ~2 s later sticks
```

Happy to provide pw-dump / wpctl dumps if useful (machine is reproducible on demand).
