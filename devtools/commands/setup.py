"""`dev.py setup`: Create .venv and install build tools."""
import os
import shutil
import subprocess
import sys

from devtools.common import (
    REQUIREMENTS, VENV_CONAN, VENV_DIR, VENV_PYTHON, run, is_venv_active,
)


def register(subparsers):
    parser = subparsers.add_parser(
        "setup", help="create .venv and install build tools (conan, cmake)"
    )
    parser.add_argument(
        "--recreate", action="store_true", help="delete and recreate .venv"
    )
    parser.set_defaults(func=execute)


def execute(args):
    if args.recreate and is_venv_active():
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
        _create_venv()
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


def _create_venv():
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