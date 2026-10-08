#!/usr/bin/env python3
"""Helper tasks for development of IcyMoon.

Run `python dev.py -h` for the list of commands.
This script only uses the Python standard library so that it can run before
the virtual environment exists.
"""

import sys
import argparse
import subprocess

MIN_PYTHON = (3, 10)

# Enforce required minimum version for Python interpreter:
# Must be run before any import of devtools to ensure a clear error message when 
# the version requirement is not met:
if sys.version_info < MIN_PYTHON:
        sys.exit(
            f"Python {'.'.join(map(str, MIN_PYTHON))}+ is required "
            f"(found {sys.version.split()[0]})"
        )

from devtools.commands import install, lock, setup

COMMANDS = [setup, install, lock]

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