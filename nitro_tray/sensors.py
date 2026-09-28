"""Read hardware sensors: coretemp, NVIDIA GPU, nbfc fan duties."""
import re
import subprocess
from pathlib import Path


def read_temps():
    """Return (cpu_c, gpu_c) — hottest coretemp package, nvidia-smi GPU."""
    cpu = 0
    for d in Path("/sys/class/hwmon").glob("hwmon*"):
        try:
            if (d / "name").read_text().strip() != "coretemp":
                continue
            for t in d.glob("temp*_input"):
                cpu = max(cpu, int(t.read_text().strip()) // 1000)
        except OSError:
            pass
    gpu = 0
    try:
        out = subprocess.run(["nvidia-smi", "--query-gpu=temperature.gpu",
                              "--format=csv,noheader"], capture_output=True,
                             text=True, timeout=3).stdout.strip().split()[0]
        gpu = int(out)
    except (OSError, ValueError, subprocess.TimeoutExpired, IndexError):
        pass
    return cpu, gpu


def read_fans():
    """Return [(name, temp, current%, target%), ...] parsed from `nbfc status`."""
    try:
        out = subprocess.run(["nbfc", "status"], capture_output=True,
                             text=True, timeout=3).stdout
    except (OSError, subprocess.TimeoutExpired):
        return []
    rows = re.findall(
        r"Fan Display Name\s*:\s*([^\n]+)\s+Temperature\s*:\s*([\d.]+)"
        r".*?Current Fan Speed\s*:\s*([\d.]+)\s+Target Fan Speed\s*:\s*([\d.]+)",
        out, re.S)
    return [(n.strip(), float(t), float(c), float(g)) for n, t, c, g in rows]
