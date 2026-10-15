"""`dev.py install`: Run `conan install` for one or more build configurations."""
import argparse
import sys

from devtools import lockfile
from devtools.common import LOCKFILE, ROOT, VENV_CONAN, export_local_recipes, require_venv_active, run
from devtools.configs import CONFIGS, CURRENT_OS, DEFAULT_GROUP, GROUPS, Config, describe, supported_configs


def register(subparsers):
    parser = subparsers.add_parser(
        "install",
        help="install dependencies and generate CMake presets",
        epilog=describe(),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "names",
        nargs="*",
        metavar="config",
        help=f"configurations or groups to install (default: {DEFAULT_GROUP})",
    )
    parser.add_argument(
        "--all", action="store_true", help="install every configuration for this OS"
    )
    parser.set_defaults(func=execute)


def execute(args):
    require_venv_active()
    if args.all and args.names:
        sys.exit("--all installs every configuration: don't combine it with configuration names")
    names = supported_configs() if args.all else _resolve(args.names or [DEFAULT_GROUP])
    unsupported = [name for name in names if not CONFIGS[name].is_supported]
    if unsupported:
        sys.exit(f"Can't install on {CURRENT_OS}: {', '.join(unsupported)}\n\n{describe()}")

    # conan.lock pins the revision of local recipes, so edits to them would be ignored
    # (or fail to resolve on a fresh machine) until the lockfile is updated
    local_references = export_local_recipes()
    if LOCKFILE.exists():
        outdated = lockfile.changed_packages(lockfile.read_entries(), local_references)
        if outdated:
            sys.exit(
                f"conan.lock pins an outdated revision of: {', '.join(outdated)}\n"
                "Run `python3 dev.py lock` to update it, then commit conan.lock."
            )

    for name in names:
        _install(name, CONFIGS[name])

    print(
        f"\nInstalled: {', '.join(names)}\n"
        "List the available presets with `cmake --list-presets`, then for example:\n"
        f"    cmake --preset {names[0]}\n"
        f"    cmake --build --preset {names[0]}\n"
    )


def _resolve(names: list[str]) -> list[str]:
    """Expand groups into configuration names, keeping order and dropping duplicates."""
    resolved = []
    for name in names:
        if name in GROUPS:
            candidates = GROUPS[name]
        elif name in CONFIGS:
            candidates = [name]
        else:
            sys.exit(f"Unknown configuration or group: '{name}'\n\n{describe()}")
        for candidate in candidates:
            if candidate not in resolved:
                resolved.append(candidate)
    return resolved


def _install(name: str, config: Config):
    print(f"\n=== {name} ===", flush=True)
    run([VENV_CONAN, "install", ROOT, *config.profile_args(), "--build=missing"])