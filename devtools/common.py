"""Paths and helpers shared by all devtools."""
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REQUIREMENTS = ROOT / "requirements.txt"

VENV_DIR = ROOT / ".venv"
VENV_BIN = VENV_DIR / ("Scripts" if os.name == "nt" else "bin")
VENV_PYTHON = VENV_BIN / ("python.exe" if os.name == "nt" else "python")
VENV_CONAN = VENV_BIN / ("conan.exe" if os.name == "nt" else "conan")

def run(cmd: list[str | Path], **kwargs) -> None:
    """Print a command, then run it from the project root. Raises on failure."""
    processed_cmd = [str(c) for c in cmd]
    print(">>", " ".join(processed_cmd), flush=True)
    subprocess.run(processed_cmd, cwd=ROOT, check=True, **kwargs)

def is_venv_active() -> bool:
    """True if this script is executed by the interpreter of .venv."""
    return Path(sys.prefix).resolve() == VENV_DIR.resolve()

def require_venv() -> None:
    """Exit with a helpful message if `dev.py setup` has not been run."""
    if not VENV_CONAN.exists():
        sys.exit("Build tools not found. Run `python3 dev.py setup` first.")