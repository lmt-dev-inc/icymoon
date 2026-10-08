"""`dev.py lock`: Create or update conan.lock for every build configuration.

The lockfile pins the exact version and recipe revision of every dependency.
It covers all configurations at once: each `conan install` only uses the
entries relevant to its configuration. Conan picks up conan.lock automatically,
so `dev.py install` needs no extra argument.
"""
import json
import tempfile
from pathlib import Path

from devtools.common import LOCKFILE, ROOT, VENV_CONAN, export_local_recipes, require_venv_active, run
from devtools.configs import CONFIGS


def register(subparsers):
    parser = subparsers.add_parser(
        "lock",
        help="create or update conan.lock for every configuration",
        description="Without options, adds missing entries to conan.lock and removes the "
        "ones no configuration uses: locked dependencies are never upgraded. Local recipes under recipes/ are "
        "always relocked so that edits to them are picked up.",
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--update",
        nargs="+",
        metavar="package",
        help="upgrade these packages to the latest version and revision allowed by conanfile.py "
        "(e.g. `--update gdal proj`)",
    )
    mode.add_argument(
        "--recreate",
        action="store_true",
        help="delete conan.lock and lock every package to its latest version and revision",
    )
    parser.set_defaults(func=execute)


def execute(args):
    require_venv_active()

    previous_entries = _read_entries() if LOCKFILE.exists() else []
    if args.recreate and LOCKFILE.exists():
        print(f"Removing {LOCKFILE}")
        LOCKFILE.unlink()

    local_references = export_local_recipes()

    # Entries removed from the lockfile are resolved again by `conan lock create`,
    # while all other entries stay pinned
    if LOCKFILE.exists():
        packages = _changed_packages(previous_entries, local_references) + (args.update or [])
        if packages:
            _remove_from_lockfile(packages)

    # Check the remotes for newer versions and revisions of the packages being resolved
    update_args = ["--update"] if args.recreate or args.update else []

    # Each configuration is locked separately with --lockfile-clean, still constrained by
    # conan.lock so that pinned entries are kept. Entries used by no configuration are
    # absent from every per-configuration lockfile, so the merge drops them.
    lockfile_args = ["--lockfile", LOCKFILE] if LOCKFILE.exists() else []
    with tempfile.TemporaryDirectory() as tmp:
        config_lockfiles = []
        for name, config in CONFIGS.items():
            print(f"\n=== {name} ===", flush=True)
            config_lockfile = Path(tmp) / f"{name}.lock"
            run([
                VENV_CONAN, "lock", "create", ROOT,
                *config.profile_args(),
                *lockfile_args,
                "--lockfile-out", config_lockfile,
                "--lockfile-clean",
                *update_args,
            ])
            config_lockfiles.append(config_lockfile)

        merge_args = [arg for lockfile in config_lockfiles for arg in ("--lockfile", lockfile)]
        run([VENV_CONAN, "lock", "merge", *merge_args, "--lockfile-out", LOCKFILE])

    _restore_timestamps(previous_entries, local_references)
    print(f"\nUpdated {LOCKFILE.name}. Review the changes with `git diff {LOCKFILE.name}` and commit them.")


def _read_entries() -> list[str]:
    """Entries of the lockfile, of the form "name/version#revision%timestamp"."""
    lockfile = json.loads(LOCKFILE.read_text())
    return lockfile.get("requires", []) + lockfile.get("build_requires", []) + lockfile.get("python_requires", [])


def _changed_packages(entries: list[str], references: list[str]) -> list[str]:
    """Names of the locked packages whose exported revision differs from the one in the lockfile.

    Packages that aren't locked at all (e.g. open_usd, which nothing requires) need no
    removal: `conan lock create` adds them if a configuration requires them.
    """
    locked_references = {entry.split("%")[0] for entry in entries}
    locked_names = {entry.split("/")[0] for entry in entries}
    return [
        reference.split("/")[0]
        for reference in references
        if reference.split("/")[0] in locked_names and reference not in locked_references
    ]


def _remove_from_lockfile(packages: list[str]):
    """Remove all versions of the given packages, whether used as requires or tool requires."""
    cmd = [VENV_CONAN, "lock", "remove", "--lockfile", LOCKFILE, "--lockfile-out", LOCKFILE]
    for package in packages:
        pattern = package if "/" in package else f"{package}/*"
        cmd += ["--requires", pattern, "--build-requires", pattern]
    run(cmd)


def _restore_timestamps(previous_entries: list[str], local_references: list[str]):
    """Keep the previous timestamp of local recipes whose revision hasn't changed.

    Exporting a local recipe gives it a new timestamp even when its revision is
    unchanged, and `conan lock create` copies that timestamp into the lockfile.
    Restoring it ensures conan.lock only changes when a revision actually changes.
    Remote packages are left untouched: their timestamp is set by the server.
    """
    local = set(local_references)
    previous = {
        entry.split("%")[0]: entry
        for entry in previous_entries
        if entry.split("%")[0] in local
    }
    lockfile = json.loads(LOCKFILE.read_text())
    for section in ("requires", "build_requires", "python_requires"):
        if section in lockfile:
            lockfile[section] = [previous.get(entry.split("%")[0], entry) for entry in lockfile[section]]
    # Same formatting as Conan, so that only actual changes show in the diff
    LOCKFILE.write_text(json.dumps(lockfile, indent=4) + "\n")
