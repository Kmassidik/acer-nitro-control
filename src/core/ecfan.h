#pragma once
// Direct-EC fan engine (replaces nbfc entirely).
// Verified protocol on Acer Nitro AN515-58 (decoded from nbfc /dev/port trace):
//   EC ports: cmd 0x66, data 0x62. Read cmd 0x80, write cmd 0x81.
//   Unlock:   reg 0x03 = 0x51 (REQUIRED before manual-mode/duty writes)
//   Manual:   reg 0x34 = 0x0C (CPU), reg 0x33 = 0x30 (GPU)
//   Duty:     reg 0x37 = pct (CPU), reg 0x3A = pct (GPU), 0..100 (%)
//   Tach:     word reg 0x13/0x14 (CPU), 0x15/0x16 (GPU) — duty-of-8500 scale
//   Auto:     reg 0x34 = 0x00, reg 0x33 = 0x00 (curve inside EC firmware)
// Root-only (ioperm). UI talks to it via control::setFanPct/setFansAuto which
// delegate to the nitro-priv root helper — see core/ecfan_cli notes in control.cpp.
namespace ecfan {
bool init(void);
int  duty(int fanIdx);          // 0..100, -1 on failure
void setManual(int pct);        // both fans
void setAuto(void);
int  tempC(void);               // EC CPU temp, -1 unknown
} // namespace ecfan
