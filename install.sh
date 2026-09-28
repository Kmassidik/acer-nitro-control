#!/usr/bin/env bash
# install.sh — build & install nitro-control (C++/Qt6) — run from repo root.
# System parts (fan config, services) need sudo.
set -e
D="$(cd "$(dirname "$0")" && pwd)"

echo "[1/7] build (cmake)"
cmake -S "$D" -B "$D/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$D/build" -j"$(nproc)"

echo "[2/7] binary -> ~/.local/bin/nitro-control"
mkdir -p ~/.local/bin
cp "$D/build/nitro-control" ~/.local/bin/nitro-control
chmod +x ~/.local/bin/nitro-control

echo "[3/7] helpers -> ~/.local/bin + /usr/local/bin"
cp "$D/bin/turbo-lvl" ~/.local/bin/turbo-lvl
chmod +x ~/.local/bin/turbo-lvl
sudo cp "$D/bin/nitro-thermal-guard" /usr/local/bin/nitro-thermal-guard
sudo chmod +x /usr/local/bin/nitro-thermal-guard

echo "[4/7] fan curve -> nbfc config (backs up old)"
CFG="/usr/local/share/nbfc/configs/Acer Nitro AN515-58.json"
sudo cp "$CFG" "$CFG.bak.$(date +%s)" 2>/dev/null || true
sudo cp "$D/nbfc/Acer-Nitro-AN515-58.json" "$CFG"

echo "[5/7] services -> systemd + enable at boot"
sudo cp "$D/systemd/nitro-thermal.service" /etc/systemd/system/nitro-thermal.service
mkdir -p ~/.config/systemd/user
cp "$D/systemd/nitro-tray.service" ~/.config/systemd/user/nitro-tray.service
cp "$D/systemd/nitro-rgb-restore.service" ~/.config/systemd/user/nitro-rgb-restore.service
sudo mkdir -p /usr/lib/systemd/system-sleep
sudo cp "$D/systemd/system-sleep/nitro-rgb" /usr/lib/systemd/system-sleep/nitro-rgb
sudo chmod 755 /usr/lib/systemd/system-sleep/nitro-rgb
# systemd user service is the ONLY tray launcher (two mechanisms = 2 tray icons bug)
rm -f ~/.config/autostart/nitro-tray.desktop
sudo systemctl daemon-reload
sudo systemctl enable --now nbfc_service nitro-thermal
systemctl --user daemon-reload
systemctl --user restart nitro-tray
systemctl --user enable --now nitro-rgb-restore

echo "[6/7] verify"
systemctl is-enabled nbfc_service nitro-thermal
systemctl --user is-enabled nitro-tray nitro-rgb-restore
systemctl --user is-active nitro-tray
nbfc status | grep -E "Temperature|Target Fan" || true
test -x /usr/lib/systemd/system-sleep/nitro-rgb && echo "resume hook: OK"

echo "[7/7] apply saved keyboard RGB"
~/.local/bin/nitro-control --restore-rgb && echo "rgb state applied" || echo "rgb: no state yet (set it once from the RGB panel)"
echo OK
