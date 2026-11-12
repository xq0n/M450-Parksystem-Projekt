"""Build and run the SAME C++ core/protocol/controller used in the Pico firmware."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
compiler = os.environ.get("CXX") or shutil.which("g++") or shutil.which("clang++")
if not compiler:
    sys.exit("C++ compiler missing. Install GCC or Clang and add it to PATH (see README).")
out = root / "build" / "host"
out.mkdir(parents=True, exist_ok=True)
binary = out / ("test_parking.exe" if os.name == "nt" else "test_parking")
cmd = [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic", "-O1", "-g",
       "-I", str(root / "include"), str(root / "test" / "test_parking.cpp"), "-o", str(binary)]
if "--sanitize" in sys.argv:
    cmd += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
subprocess.run(cmd, check=True, cwd=root)
env = os.environ.copy()
env["PATH"] = str(Path(compiler).resolve().parent) + os.pathsep + env.get("PATH", "")
subprocess.run([str(binary)], check=True, cwd=root, env=env)
