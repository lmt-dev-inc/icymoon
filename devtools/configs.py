"""Build configurations available to dev.py commands.

A configuration maps to one or two Conan profiles under profiles/:
- host: the platform the code is built for
- build: the platform running the build tools. Only needed when cross-compiling
  (e.g. Android); None means the host profile is used for both (-pr:a).

Configuration names match the CMake preset names produced by Conan, so that
`dev.py install lnx-gcc-debug` is followed by `cmake --preset lnx-gcc-debug`.
"""
import os
from dataclasses import dataclass
from pathlib import Path

from devtools.common import ROOT

PROFILES_DIR = ROOT / "profiles"


@dataclass(frozen=True)
class Config:
    host: str
    build: str | None = None

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

DEFAULT_GROUP = "win" if os.name == "nt" else "lnx"
