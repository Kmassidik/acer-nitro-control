# alsa-ucm-conf PR draft / issue

**Repo:** https://github.com/alsa-project/alsa-ucm-conf
**File:** `ucm2/Intel/sof-hda-dsp/sof-hda-dsp.conf` (+ HiFi verbiage)

**Title:** `sof-hda-dsp: expose Headphones as a jack-conditional port inside the Speaker HiFi profile`

## Problem

On Intel `skl_hda_dsp_generic` (observed on Acer Nitro AN515-58), the current UCM
splits Speaker and Headphones into two exclusive HiFi profiles:

- `HiFi (HDMI1, HDMI2, HDMI3, Mic1, Mic2, Speaker)`
- `HiFi (HDMI1, HDMI2, HDMI3, Headphones, Mic1, Mic2)`

Consequences on Linux (PipeWire/WirePlumber):

1. **Headset plugged in after boot is invisible** — the running profile contains
   no Headphones port, so port-availability events never fire; users must switch
   profiles manually or run a watcher (kernel knows: `Headphone Jack = on`).
2. Profile juggling around the jack trips consumers' profile-restore policies
   (WirePlumber) → card can drift to `pro-audio` → silent system.
3. Windows drivers (StarbookSame HW) present the 3.5 mm as a port of one stereo
   device; only Linux needs the two-profile workaround.

## Proposal

Fold Headphones into the main HiFi profile as a jack-gated port, mirroring what
`ucm2/HDAudio` and several laptop UCMs already do:

- keep `Speaker` and `Headphones` both mapped in the same `HiFi` verbiage,
- gate `Headphones` with `JackControl "Headphone Jack"` and
  `JackHWMute "Speaker"` so availability flips automatically at plug/unplug,
- drop the dedicated Headphones-only profile (or keep it for compat with
  older pipelines but give transition the same BCM gate).

Sketch (style-details per repo conventions):

```
# sof-hda-dsp "HiFi" verbiage
SectionDevice."Headphones" {
    Value {
        PlaybackChannels "2"
        JackControl "Headphone Jack"
        JackHWMute "Speaker"     # HW-dir: HP amp covers the jack line
    }
    EnableSequence [...]
    DisableSequence [...]
}
```

## Tested hardware

Acer Nitro AN515-58, BIOS V1.11, kernel 7.2.x Fedora — `Headphone Jack` control
(numid=12, `amixer -c1 contents`) flips reliably with plug/unplug, both jacked
routes (`Headphone Playback Switch/Volume`) verified working manually:

```
/proc/asound/card1/codec#0: Pin Default 0x04211020: [Jack] HP Out at Ext Right
```

I can prepare the full UCM diff against `master` and test it live on this
machine before submission — please advise if folding ports into one HiFi
profile conflicts with any documented behavior for this topology (HDMI + DSP
hybrid), since this UCM was shared across many Intel HDA+HDMI platforms.
