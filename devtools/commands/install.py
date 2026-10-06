"""`dev.py install`: Run `conan install` for one or more build configurations."""
import sys

from devtools.common import ROOT, VENV_CONAN, require_venv, run
from devtools.configs import CONFIGS, DEFAULT_GROUP, GROUPS, PROFILES_DIR, Config

RECIPES_DIR = ROOT / "recipes"


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
    require_venv()
    names = list(CONFIGS) if args.all else _resolve(args.names or [DEFAULT_GROUP])

    _export_local_recipes()
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


def _export_local_recipes():
    """Add our own recipes (anari, open_usd) to the Conan cache.

    Exporting is fast and does nothing if a recipe hasn't changed. Packages are
    then built on demand by `--build=missing`, only if a configuration needs them.
    """
    for conanfile in sorted(RECIPES_DIR.glob("*/conanfile.py")):
        run([VENV_CONAN, "export", conanfile.parent])


def _install(name: str, config: Config):
    if config.build is None:
        profiles = ["-pr:a", PROFILES_DIR / config.host]
    else:
        profiles = ["-pr:h", PROFILES_DIR / config.host, "-pr:b", PROFILES_DIR / config.build]

    print(f"\n=== {name} ===", flush=True)
    run([VENV_CONAN, "install", ROOT, *profiles, "--build=missing"])