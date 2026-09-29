#!/usr/bin/env bash
# install.sh — build & install nitro-control (C++/Qt6) — run from repo root.
# System parts (fan config, services) need sudo.
set -e
D="$(cd "$(dirname "$0")" && pwd)"

echo "[1/8] build (cmake)"
cmake -S "$D" -B "$D/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$D/build" -j"$(nproc)"

echo "[2/8] binary -> ~/.local/bin/nitro-control"
mkdir -p ~/.local/bin
cp "$D/build/nitro-control" ~/.local/bin/nitro-control
chmod +x ~/.local/bin/nitro-control

echo "[3/8] desktop entry -> ~/.local/share/applications"
mkdir -p ~/.local/share/applications
# Qt 6.11 registers the tray icon via xdg-desktop-portal, which resolves the
# Exec line against the portal's PATH (no ~/.local/bin in it) => absolute path.
sed "s|^Exec=.*|Exec=$HOME/.local/bin/nitro-control|" "$D/desktop/nitro-control.desktop" \
    > ~/.local/share/applications/nitro-control.desktop

echo "[4/8] helpers -> ~/.local/bin + /usr/local/bin"
cp "$D/bin/turbo-lvl" ~/.local/bin/turbo-lvl
chmod +x ~/.local/bin/turbo-lvl
sudo cp "$D/bin/nitro-thermal-guard" /usr/local/bin/nitro-thermal-guard
sudo chmod +x /usr/local/bin/nitro-thermal-guard

echo "[5/8] fan curve -> nbfc config (backs up old)"
CFG="/usr/local/share/nbfc/configs/Acer Nitro AN515-58.json"
sudo cp "$CFG" "$CFG.bak.$(date +%s)" 2>/dev/null || true
sudo cp "$D/nbfc/Acer-Nitro-AN515-58.json" "$CFG"

echo "[6/8] services -> systemd + enable at boot"
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

echo "[7/8] verify"
systemctl is-enabled nbfc_service nitro-thermal
systemctl --user is-enabled nitro-tray nitro-rgb-restore
systemctl --user is-active nitro-tray
nbfc status | grep -E "Temperature|Target Fan" || true
test -x /usr/lib/systemd/system-sleep/nitro-rgb && echo "resume hook: OK"

echo "[8/8] apply saved keyboard RGB"
~/.local/bin/nitro-control --restore-rgb && echo "rgb state applied" || echo "rgb: no state yet (set it once from the RGB panel)"
echo OK
