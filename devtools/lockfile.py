"""Helpers to read conan.lock and compare it with the local recipes."""
import json

from devtools.common import LOCKFILE


def read_entries() -> list[str]:
    """Entries of the lockfile, of the form "name/version#revision%timestamp"."""
    lockfile = json.loads(LOCKFILE.read_text())
    return lockfile.get("requires", []) + lockfile.get("build_requires", []) + lockfile.get("python_requires", [])


def changed_packages(entries: list[str], references: list[str]) -> list[str]:
    """Names of the locked packages whose exported revision differs from the one in the lockfile.

    Packages that aren't locked at all (e.g. open_usd, which nothing requires) are not
    reported: `conan lock create` adds them if a configuration requires them.
    """
    locked_references = {entry.split("%")[0] for entry in entries}
    locked_names = {entry.split("/")[0] for entry in entries}
    return [
        reference.split("/")[0]
        for reference in references
        if reference.split("/")[0] in locked_names and reference not in locked_references
    ]
