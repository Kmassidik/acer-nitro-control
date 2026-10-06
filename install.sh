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

echo "[3/7] helpers -> /usr/local/bin + sudoers rule"
sudo install -m 0755 "$D/bin/nitro-priv" /usr/local/bin/nitro-priv
sudo install -m 0755 "$D/bin/turbo-lvl" /usr/local/bin/turbo-lvl
sudo install -m 0755 "$D/bin/nitro-thermal-guard" /usr/local/bin/nitro-thermal-guard
# EC fan setter: root-owned C binary (port I/O needs ioperm+root)
sudo mkdir -p /usr/local/libexec
if [ ! -x /usr/local/libexec/nitro-fan ] || [ "$D/bin/nitro-fan.c" -nt /usr/local/libexec/nitro-fan ]; then
  PATH=/usr/bin:/bin g++ -O2 "$D/bin/nitro-fan.c" -o /tmp/nitro-fan.$$ || exit 1
  sudo install -m 0755 /tmp/nitro-fan.$$ /usr/local/libexec/nitro-fan
  rm -f /tmp/nitro-fan.$$
fi
SUDOERS_FILE=/etc/sudoers.d/nitro-control
sudo tee "$SUDOERS_FILE" >/dev/null <<EOF
# nitro-control: passwordless root for the whitelisted helper only.
# Each line authorizes ONE exact argv; anything else still prompts/denies.
${SUDO_USER:-$USER} ALL=(root) NOPASSWD: /usr/local/bin/nitro-priv thermal on
${SUDO_USER:-$USER} ALL=(root) NOPASSWD: /usr/local/bin/nitro-priv thermal off
${SUDO_USER:-$USER} ALL=(root) NOPASSWD: /usr/local/bin/nitro-priv cpu *
${SUDO_USER:-$USER} ALL=(root) NOPASSWD: /usr/local/bin/nitro-priv fan *
EOF
sudo chmod 440 "$SUDOERS_FILE"
sudo visudo -c -f "$SUDOERS_FILE" || exit 1

echo "[4/7] services -> systemd + enable at boot"
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
sudo systemctl enable --now nitro-thermal
systemctl --user daemon-reload
systemctl --user restart nitro-tray
systemctl --user enable --now nitro-rgb-restore
# nbfc is no longer used: fans run via the EC helper (nitro-fan). Stop+disable
# it if present so nothing fights the EC duty writes.
systemctl is-active --quiet nbfc_service && sudo systemctl stop nbfc_service || true
systemctl is-enabled --quiet nbfc_service 2>/dev/null && sudo systemctl disable nbfc_service >/dev/null || true

echo "[5/7] verify"
systemctl is-enabled nitro-thermal
systemctl --user is-enabled nitro-tray nitro-rgb-restore
systemctl --user is-active nitro-tray
sudo -n /usr/local/bin/nitro-priv fan status && echo "fan helper: OK"
test -x /usr/lib/systemd/system-sleep/nitro-rgb && echo "resume hook: OK"

echo "[6/7] apply saved keyboard RGB"
~/.local/bin/nitro-control --restore-rgb && echo "rgb state applied" || echo "rgb: no state yet (set it once from the RGB panel)"
echo OK
