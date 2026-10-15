"""Build configurations available to dev.py commands.

A configuration maps to one or two Conan profiles under profiles/:
- host: the platform the code is built for
- build: the platform running the build tools. Only needed when cross-compiling
  (e.g. Android); None means the host profile is used for both (-pr:a).

Configuration names match the CMake preset names produced by Conan, so that
`dev.py install lnx-gcc-debug` is followed by `cmake --preset lnx-gcc-debug`.
"""
import platform
from dataclasses import dataclass
from pathlib import Path

from devtools.common import ROOT

PROFILES_DIR = ROOT / "profiles"

# Configurations are named `<os>-<compiler>-<variant>` (see profiles/README.md). Maps the <os>
# prefix to the OS that runs the build, as returned by platform.system().
OS_PREFIXES = {"lnx": "Linux", "win": "Windows"}
CURRENT_OS = platform.system()


@dataclass(frozen=True)
class Config:
    host: str
    build: str | None = None

    @property
    def os(self) -> str | None:
        """OS that runs the build of this configuration, from the prefix of its name."""
        return OS_PREFIXES.get(self.host.split("-")[0])

    @property
    def is_supported(self) -> bool:
        """True if this configuration can be built on the current OS."""
        return self.os == CURRENT_OS

    def profile_args(self) -> list[str | Path]:
        """Conan command-line arguments selecting the profiles of this configuration."""
        if self.build is None:
            return ["-pr:a", PROFILES_DIR / self.host]
        return ["-pr:h", PROFILES_DIR / self.host, "-pr:b", PROFILES_DIR / self.build]


CONFIGS: dict[str, Config] = {
    "lnx-gcc-debug": Config(host="lnx-gcc-debug"),
    "lnx-gcc-release": Config(host="lnx-gcc-release"),
    "lnx-gcc-coverage": Config(host="lnx-gcc-coverage"),
    "win-clang-debug": Config(host="win-clang-debug"),
    "win-clang-release": Config(host="win-clang-release"),
    "win-clang-coverage": Config(host="win-clang-coverage"),
}

# Shortcuts for installing several configurations at once
GROUPS: dict[str, list[str]] = {
    "lnx": ["lnx-gcc-debug", "lnx-gcc-coverage", "lnx-gcc-release"],
    "win": ["win-clang-debug", "win-clang-coverage", "win-clang-release"],
}

DEFAULT_GROUP = "win" if CURRENT_OS == "Windows" else "lnx"


def supported_configs() -> list[str]:
    """Names of the configurations that can be built on the current OS."""
    return [name for name, config in CONFIGS.items() if config.is_supported]


def supported_groups() -> dict[str, list[str]]:
    """Groups whose configurations can all be built on the current OS."""
    return {
        name: members for name, members in GROUPS.items()
        if all(CONFIGS[member].is_supported for member in members)
    }


def describe() -> str:
    """Configurations and groups available on the current OS, for command-line help."""
    lines = [f"configurations ({CURRENT_OS}):"]
    lines += [f"  {name}" for name in supported_configs()] or ["  (none)"]
    groups = supported_groups()
    if groups:
        lines += ["", "groups:"]
        width = max(len(name) for name in groups)
        for name, members in groups.items():
            default = " (default)" if name == DEFAULT_GROUP else ""
            lines.append(f"  {name:<{width}}  {', '.join(members)}{default}")
    return "\n".join(lines)
