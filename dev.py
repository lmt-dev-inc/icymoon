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

from devtools.commands import setup
from devtools.common import REQUIREMENTS, VENV_CONAN, VENV_DIR, VENV_PYTHON, is_venv_active, run

MIN_PYTHON = (3, 10)
COMMANDS = [setup]

# Enforce required minimum version for Python interpreter:
if sys.version_info < MIN_PYTHON:
        sys.exit(
            f"Python {'.'.join(map(str, MIN_PYTHON))}+ is required "
            f"(found {sys.version.split()[0]})"
        )


def main():
    parser = argparse.ArgumentParser(description="IcyMoon developer tasks")
    subparsers = parser.add_subparsers(dest="command", required=True)
    for command in COMMANDS:
        command.register(subparsers)

    args = parser.parse_args()
    try:
        args.func(args)
    except subprocess.CalledProcessError as e:
        sys.exit(f"\nCommand failed with exit code {e.returncode}")
    except KeyboardInterrupt:
        sys.exit("\nInterrupted")


if __name__ == "__main__":
    main()