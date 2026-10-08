"""`dev.py install`: Run `conan install` for one or more build configurations."""
import sys

from devtools.common import ROOT, VENV_CONAN, export_local_recipes, require_venv_active, run
from devtools.configs import CONFIGS, DEFAULT_GROUP, GROUPS, Config


def register(subparsers):
    parser = subparsers.add_parser(
        "install", help="install dependencies and generate CMake presets"
    )
    parser.add_argument(
        "names",
        nargs="*",
        metavar="config",
        help=f"configurations or groups to install (default: {DEFAULT_GROUP}). "
        f"Configurations: {', '.join(CONFIGS)}. Groups: {', '.join(GROUPS)}",
    )
    parser.add_argument(
        "--all", action="store_true", help="install every configuration"
    )
    parser.set_defaults(func=execute)


def execute(args):
    require_venv_active()
    names = list(CONFIGS) if args.all else _resolve(args.names or [DEFAULT_GROUP])

    export_local_recipes()
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
            sys.exit(
                f"Unknown configuration or group: '{name}'\n"
                f"Configurations: {', '.join(CONFIGS)}\n"
                f"Groups: {', '.join(GROUPS)}"
            )
        for candidate in candidates:
            if candidate not in resolved:
                resolved.append(candidate)
    return resolved


def _install(name: str, config: Config):
    print(f"\n=== {name} ===", flush=True)
    run([VENV_CONAN, "install", ROOT, *config.profile_args(), "--build=missing"])