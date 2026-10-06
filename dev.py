#!/usr/bin/env python3
"""Helper tasks for development of IcyMoon.

Run `python dev.py -h` for the list of commands.
This script only uses the Python standard library so that it can run before
the virtual environment exists.
"""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

MIN_PYTHON = (3, 10)

ROOT = Path(__file__).resolve().parent
REQUIREMENTS = ROOT / "requirements.txt"

VENV_DIR = ROOT / ".venv"
VENV_BIN = VENV_DIR / ("Scripts" if os.name == "nt" else "bin")
VENV_PYTHON = VENV_BIN / ("python.exe" if os.name == "nt" else "python")
VENV_CONAN = VENV_BIN / "conan"

def run(cmd, **kwargs):
    print(">>", " ".join(str(c) for c in cmd), flush=True)
    subprocess.run([str(c) for c in cmd], cwd=ROOT, check=True, **kwargs)

def create_venv():
    try:
        run([sys.executable, "-m", "venv", VENV_DIR])
    except subprocess.CalledProcessError:
        # Remove the partially created venv so the next run starts clean
        shutil.rmtree(VENV_DIR, ignore_errors=True)
        if sys.platform.startswith("linux"):
            version = f"{sys.version_info.major}.{sys.version_info.minor}"
            print(
                "\nFailed to create the virtual environment.\n"
                "On Debian/Ubuntu, the venv module is packaged separately. Install it with:\n"
                f"    sudo apt install python3-venv  (or python{version}-venv)\n",
                file=sys.stderr,
            )
        raise

def running_in_project_venv():
    """True if this script is executed by the interpreter of .venv."""
    return Path(sys.prefix).resolve() == VENV_DIR.resolve()

def cmd_setup(args):
    if args.recreate and running_in_project_venv():
        sys.exit(
            "Cannot recreate .venv while running from it.\n"
            "Run `deactivate` first, then run `python3 dev.py setup --recreate` again."
        )
        
    if args.recreate and VENV_DIR.exists():
        print(f"Removing {VENV_DIR}")
        shutil.rmtree(VENV_DIR)

    # Check for the interpreter rather than the folder: a broken or partial
    # venv folder should be recreated
    if not VENV_PYTHON.exists():
        create_venv()
    else:
        print(f"Using existing virtual environment: {VENV_DIR}")

    run([VENV_PYTHON, "-m", "pip", "install", "--upgrade", "pip"])
    run([VENV_PYTHON, "-m", "pip", "install", "-r", REQUIREMENTS])
    run([VENV_CONAN, "profile", "detect", "--exist-ok"])

    activate = (
        r".venv\Scripts\activate" if os.name == "nt" else "source .venv/bin/activate"
    )
    print(
        "\nSetup complete.\n"
        "To use conan and cmake from a terminal, activate the virtual environment:\n"
        f"    {activate}\n"
    )

def main():
    if sys.version_info < MIN_PYTHON:
        sys.exit(
            f"Python {'.'.join(map(str, MIN_PYTHON))}+ is required "
            f"(found {sys.version.split()[0]})"
        )

    parser = argparse.ArgumentParser(description="IcyMoon developer tasks")
    subparsers = parser.add_subparsers(dest="command", required=True)

    setup_parser = subparsers.add_parser(
        "setup", help="Create .venv and install build tools (conan, cmake)"
    )
    setup_parser.add_argument(
        "--recreate", action="store_true", help="Delete and recreate .venv"
    )
    setup_parser.set_defaults(func=cmd_setup)

    args = parser.parse_args()
    try:
        args.func(args)
    except subprocess.CalledProcessError as e:
        sys.exit(f"\nCommand failed with exit code {e.returncode}")
    except KeyboardInterrupt:
        sys.exit("\nInterrupted")


if __name__ == "__main__":
    main()