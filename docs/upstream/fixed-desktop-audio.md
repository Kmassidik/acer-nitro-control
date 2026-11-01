# How we fixed the desktop audio bugs (AN515-58, Fedora 44)

Three stacked bugs caused "headset never shows up" and "silence after
unplug". Here's each bug and the exact code that fixes it locally.

## Bug 1 — headset plugged in after boot is invisible

**Where:** ALSA UCM (`sof-hda-dsp`) — Speaker and Headphones are
**exclusive HiFi profiles**. The Headphones port doesn't exist inside
the Speaker profile, so a plug event after boot fires nothing and the
card is never re-evaluated (`/proc/asound/card1` still shows
`Headphone Jack = on`, PipeWire shows no Headphones sink).

**Fix:** `bin/hdajack-profile` — poll the kernel jack switch and apply
the matching profile:

```bash
# find the analog card (index changes between boots)
for c in /proc/asound/card[0-9]*; do
  idx=${c#/proc/asound/card}
  amixer -c "$idx" contents 2>/dev/null | grep -q "name='Headphone Jack'" && break
done

HP_PROFILE="HiFi (HDMI1, HDMI2, HDMI3, Headphones, Mic1, Mic2)"
SP_PROFILE="HiFi (HDMI1, HDMI2, HDMI3, Mic1, Mic2, Speaker)"
CARDNAME=alsa_card.pci-0000_00_1f.3-platform-skl_hda_dsp_generic

case $state in
  on)  pactl set-card-profile "$CARDNAME" "$HP_PROFILE" ;;
  off) pactl set-card-profile "$CARDNAME" "$SP_PROFILE" ;;
esac
```

## Bug 2 — silence after unplug: profile races to `pro-audio`

**Where:** WirePlumber `state-profile` restore policy. After an external
`pactl set-card-profile`, the saved (stale) profile re-applies and beats
ours; the card drifts onto the raw multi-channel "Pro" sinks (silent).

**Fix:** in the same watcher — **set, wait 2 s, re-assert** (now wait;
the second write wins the race), then rebind the effects chain:

```bash
pactl set-card-profile "$CARDNAME" "$want"
sleep 2
pactl set-card-profile "$CARDNAME" "$want"   # re-assert
systemctl --user try-restart easyeffects.service
```

## Bug 3 — EasyEffects latch: EQ keeps routing to a dead node

**Where:** EasyEffects trusts the last-bound node; when the profile
switch removes the Headphones node, its `easyeffects_sink` keeps
existing but routes nowhere → default sink silent.

**Fix:** `try-restart easyeffects` after every profile change (line 2
of Bug 2's snippet) + the `systemd/easyeffects.service` autostart with
a **retry loop** — the first `-l Boosted` preset load raced
GApplication registration and was silently lost:

```ini
[Service]
Type=simple
ExecStart=/usr/bin/easyeffects --service-mode
ExecStartPost=/bin/sh -c 'for i in 1 2 3 4 5; do \
  timeout 8 /usr/bin/easyeffects -l Boosted 2>/dev/null && exit 0; \
  sleep 1; done; exit 0'
Restart=on-failure
```

## Install

```bash
cp bin/hdajack-profile ~/.local/bin/ && chmod +x ~/.local/bin/hdajack-profile
cp systemd/hdajack-profile.service systemd/easyeffects.service \
   ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now hdajack-profile easyeffects
```

Presets: `~/.local/share/easyeffects/{output,irs}` (official
JackHack96/EasyEffects-Presets pack; "Boosted" is the default here).

## What we ask upstream

(Surplus to this local workaround — see `docs/upstream/`.)
- **WirePlumber**: react to jack events crossing the UCM profile
  boundary; verify stale restore state; keep external switches sticky.
- **alsa-ucm-conf**: fold Headphones into the HiFi profile as a
  jack-gated port (`JackControl "Headphone Jack"` +
  `JackHWMute "Speaker"`), like `ucm2/HDAudio` already does — then no
  watcher should be needed on any sof-hda laptop.
