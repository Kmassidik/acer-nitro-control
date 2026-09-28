#!/usr/bin/env bash
# install.sh — install nitro-control (run from repo root; needs sudo)
set -e
D="$(cd "$(dirname "$0")" && pwd)"

echo "[1/6] tray package -> ~/.local/share/nitro-control"
mkdir -p ~/.local/share/nitro-control ~/.local/bin
rm -rf ~/.local/share/nitro-control/nitro_tray
cp -r "$D/nitro_tray" ~/.local/share/nitro-control/nitro_tray
find ~/.local/share/nitro-control -name __pycache__ -type d -exec rm -rf {} + 2>/dev/null || true

echo "[2/6] launchers -> ~/.local/bin + /usr/local/bin"
cp "$D/bin/turbo-lvl" ~/.local/bin/turbo-lvl
cp "$D/bin/nitro-tray" ~/.local/bin/nitro-tray
cp "$D/bin/nitro-rgb-restore" ~/.local/bin/nitro-rgb-restore
chmod +x ~/.local/bin/turbo-lvl ~/.local/bin/nitro-tray ~/.local/bin/nitro-rgb-restore
rm -f ~/.local/bin/nitro-tray.py   # legacy single-file version
sudo cp "$D/bin/nitro-thermal-guard" /usr/local/bin/nitro-thermal-guard
sudo chmod +x /usr/local/bin/nitro-thermal-guard

echo "[3/6] fan curve -> nbfc config (backs up old)"
CFG="/usr/local/share/nbfc/configs/Acer Nitro AN515-58.json"
sudo cp "$CFG" "$CFG.bak.$(date +%s)" 2>/dev/null || true
sudo cp "$D/nbfc/Acer-Nitro-AN515-58.json" "$CFG"

echo "[4/6] services -> systemd + enable at boot"
sudo cp "$D/systemd/nitro-thermal.service" /etc/systemd/system/nitro-thermal.service
mkdir -p ~/.config/systemd/user
cp "$D/systemd/nitro-tray.service" ~/.config/systemd/user/nitro-tray.service
cp "$D/systemd/nitro-rgb-restore.service" ~/.config/systemd/user/nitro-rgb-restore.service
# resume hook: re-apply keyboard RGB after suspend
sudo mkdir -p /usr/lib/systemd/system-sleep
sudo cp "$D/systemd/system-sleep/nitro-rgb" /usr/lib/systemd/system-sleep/nitro-rgb
sudo chmod 755 /usr/lib/systemd/system-sleep/nitro-rgb
# systemd user service is the ONLY tray launcher (two mechanisms = 2 tray icons bug)
rm -f ~/.config/autostart/nitro-tray.desktop
sudo systemctl daemon-reload
sudo systemctl enable --now nbfc_service nitro-thermal
systemctl --user daemon-reload
systemctl --user enable --now nitro-tray nitro-rgb-restore

echo "[5/6] verify"
systemctl is-enabled nbfc_service nitro-thermal
systemctl --user is-enabled nitro-tray nitro-rgb-restore
nbfc status | grep -E "Temperature|Target Fan"
test -x /usr/lib/systemd/system-sleep/nitro-rgb && echo "resume hook: OK"

echo "[6/6] apply saved keyboard RGB"
~/.local/bin/nitro-rgb-restore && echo "rgb state applied" || echo "rgb: no state yet (set it once from the widget)"
echo OK
