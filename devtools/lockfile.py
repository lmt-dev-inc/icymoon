"""Helpers to read and write conan.lock and compare it with the local recipes."""
import json
from pathlib import Path
from typing import Callable

from devtools.common import LOCKFILE

# Lockfile sections listing entries of the form "name/version#revision%timestamp"
SECTIONS = ("requires", "build_requires", "python_requires")


def strip_timestamp(entry: str) -> str:
    """"name/version#revision%timestamp" -> "name/version#revision"."""
    return entry.split("%")[0]


def package_name(entry: str) -> str:
    """Package name of a lockfile entry or recipe reference."""
    return entry.split("/")[0]


def read_entries(path: Path = LOCKFILE) -> list[str]:
    """Entries of the lockfile, of the form "name/version#revision%timestamp"."""
    content = json.loads(path.read_text())
    return [entry for section in SECTIONS for entry in content.get(section, [])]


def rewrite_entries(transform: Callable[[str], str], path: Path = LOCKFILE):
    """Replace every entry of the lockfile with `transform(entry)`."""
    content = json.loads(path.read_text())
    for section in SECTIONS:
        if section in content:
            content[section] = [transform(entry) for entry in content[section]]
    # Same formatting as Conan, so that only actual changes show in the diff
    path.write_text(json.dumps(content, indent=4) + "\n")


def changed_packages(entries: list[str], references: list[str]) -> list[str]:
    """Names of the locked packages whose exported revision differs from the one in the lockfile.

    Packages that aren't locked at all (e.g. open_usd, which nothing requires) are not
    reported: `conan lock create` adds them if a configuration requires them.
    """
    locked_references = {strip_timestamp(entry) for entry in entries}
    locked_names = {package_name(entry) for entry in entries}
    return [
        package_name(reference)
        for reference in references
        if package_name(reference) in locked_names and reference not in locked_references
    ]
