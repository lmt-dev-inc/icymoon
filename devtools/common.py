"""Paths and helpers shared by all devtools."""
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REQUIREMENTS = ROOT / "requirements.txt"
RECIPES_DIR = ROOT / "recipes"
LOCKFILE = ROOT / "conan.lock"

VENV_DIR = ROOT / ".venv"
VENV_BIN = VENV_DIR / ("Scripts" if os.name == "nt" else "bin")
VENV_PYTHON = VENV_BIN / ("python.exe" if os.name == "nt" else "python")
VENV_CONAN = VENV_BIN / ("conan.exe" if os.name == "nt" else "conan")

def run(cmd: list[str | Path], **kwargs) -> subprocess.CompletedProcess:
    """Print a command, then run it from the project root. Raises on failure.

    Extra keyword arguments are passed to `subprocess.run`.
    """
    processed_cmd = [str(c) for c in cmd]
    print(">>", " ".join(processed_cmd), flush=True)
    return subprocess.run(processed_cmd, cwd=ROOT, check=True, **kwargs)

def is_venv_active() -> bool:
    """True if this script is executed by the interpreter of .venv."""
    return Path(sys.prefix).resolve() == VENV_DIR.resolve()

def require_venv_active() -> None:
    """Exit with a helpful message if `dev.py setup` has not been run."""
    if not VENV_CONAN.exists():
        sys.exit("Build tools not found. Run `python3 dev.py setup` first.")
    if not is_venv_active():
        sys.exit(f"Virtual environment not active. Run `source {VENV_DIR.relative_to(ROOT)}/bin/activate` first.")

def export_local_recipes() -> list[str]:
    """Add our own recipes (anari, open_usd) to the Conan cache and return their references.

    Exporting is fast and keeps the same revision if a recipe hasn't changed. Packages
    are then built on demand by `--build=missing`, only if a configuration needs them.
    References have the form "name/version#revision", e.g. "anari/0.14.1#e2c0e73c...".
    """
    references = []
    for conanfile in sorted(RECIPES_DIR.glob("*/conanfile.py")):
        result = run([VENV_CONAN, "export", conanfile.parent, "--format=json"])
        output = result.stdout
        references.append(json.loads(output)["reference"])
    return references