"""Build configurations available to dev.py commands.

A configuration maps to one or two Conan profiles under profiles/:
- host: the platform the code is built for
- build: the platform running the build tools. Only needed when cross-compiling
  (e.g. Android); None means the host profile is used for both (-pr:a).

Configuration names match the CMake preset names produced by Conan, so that
`dev.py install gcc-debug` is followed by `cmake --preset gcc-debug`.
"""
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
    "gcc-debug": Config(host="gcc-debug"),
    "gcc-release": Config(host="gcc-release"),
    "gcc-coverage_on-debug": Config(host="gcc-coverage_on-debug"),
}

# Shortcuts for installing several configurations at once
GROUPS: dict[str, list[str]] = {
    "gcc": ["gcc-debug", "gcc-coverage_on-debug", "gcc-release"],
}

DEFAULT_GROUP = "gcc"
