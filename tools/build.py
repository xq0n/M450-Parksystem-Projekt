"""Build all board/role combinations; optionally use the verified local PIO cache."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--offline", action="store_true", help="Use .pio-core platform at the pinned commit")
parser.add_argument("-e", "--environment", action="append", help="Build only this PlatformIO environment")
args = parser.parse_args()
pio = shutil.which("pio") or str(Path.home() / ".platformio" / "penv" / ("Scripts/pio.exe" if os.name == "nt" else "bin/pio"))
env = os.environ.copy()
command = [pio, "run"]
if args.offline:
    platform = root / ".pio-core" / "platforms" / "raspberrypi"
    commit = "5d4561a05e3b212660ac6fdd3fbfb328d1988aa1"
    actual = subprocess.check_output(["git", "-C", str(platform), "rev-parse", "HEAD"], text=True).strip()
    if actual != commit:
        raise SystemExit("Offline platform differs from pinned revision; use a regular online build.")
    config = (root / "platformio.ini").read_text(encoding="utf-8")
    config = config.replace("https://github.com/maxgerhardt/platform-raspberrypi.git#" + commit, platform.as_posix())
    tmp = root / "tmp"
    tmp.mkdir(exist_ok=True)
    config_path = tmp / "platformio-offline.ini"
    config_path.write_text(config, encoding="utf-8")
    command += ["-c", str(config_path)]
    env["PLATFORMIO_CORE_DIR"] = str(root / ".pio-core")
for name in args.environment or ["pico1-kasse", "pico2-schranken", "pico1-kasse-rp2350", "pico2-schranken-rp2350"]:
    command += ["-e", name]
subprocess.run(command, cwd=root, env=env, check=True)
