#!/usr/bin/env bash
# install.sh — install nitro-control into system paths + enable autostart (run from this folder)
set -e
D="$(cd "$(dirname "$0")" && pwd)"
echo "[1/4] scripts -> ~/.local/bin + /usr/local/bin"
mkdir -p ~/.local/bin
cp "$D/turbo-lvl" ~/.local/bin/turbo-lvl
cp "$D/nitro-tray.py" ~/.local/bin/nitro-tray.py
chmod +x ~/.local/bin/turbo-lvl ~/.local/bin/nitro-tray.py
echo "18081995" | sudo -S cp "$D/nitro-thermal-guard" /usr/local/bin/nitro-thermal-guard
echo "18081995" | sudo -S chmod +x /usr/local/bin/nitro-thermal-guard
echo "[2/4] fan curve -> nbfc configs (backs up old)"
echo "18081995" | sudo -S cp "/usr/local/share/nbfc/configs/Acer Nitro AN515-58.json" "/usr/local/share/nbfc/configs/Acer Nitro AN515-58.json.bak.$(date +%s)"
echo "18081995" | sudo -S cp "$D/Acer-Nitro-AN515-58.json" "/usr/local/share/nbfc/configs/Acer Nitro AN515-58.json"
echo "[3/4] services -> systemd + enable at boot"
echo "18081995" | sudo -S cp "$D/nitro-thermal.service" /etc/systemd/system/nitro-thermal.service
mkdir -p ~/.config/systemd/user
cp "$D/nitro-tray.service" ~/.config/systemd/user/nitro-tray.service
echo "18081995" | sudo -S systemctl daemon-reload
echo "18081995" | sudo -S systemctl enable --now nbfc_service nitro-thermal
systemctl --user daemon-reload
systemctl --user enable --now nitro-tray
mkdir -p ~/.config/autostart
cat > ~/.config/autostart/nitro-tray.desktop <<EOF
[Desktop Entry]
Name=Nitro Thermal Tray
Exec=/home/kurnia/.local/bin/nitro-tray.py
Icon=computer
Terminal=false
Type=Application
X-GNOME-Autostart-enabled=true
EOF
echo "[4/4] verify"
systemctl is-enabled nbfc_service nitro-thermal
systemctl --user is-enabled nitro-tray
nbfc status | grep -E "Temperature|Target Fan"
echo OK
