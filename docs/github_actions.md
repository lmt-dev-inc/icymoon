# GitHub Actions

The workflows under `.github/workflows/` separate *when* checks run from *what* they do:

- **Trigger files** (`pr.yml`, `main.yml`, ...) only hold an event (pull request, push, schedule)
  and the list of job files to call with their inputs.
- **Job files** (`_build.yml`, ...) hold the actual work. They only have the `workflow_call`
  event, so they never run on their own.

Several triggers can then run the same jobs, each with its own conditions and configurations,
without duplicating any steps.

## Layout

```
.github/
  actions/
    setup/action.yml   # composite action: steps shared by job files
  workflows/
    pr.yml             # trigger: pull requests to main
    main.yml           # trigger: pushes to main
    scheduled.yml      # trigger: Monday and Thursday at 06:00 UTC, or manually
    _build.yml         # job: configure, build and run unit tests, one matrix entry per configuration
```

| File            | Name in GitHub | Runs on                                    | Jobs    |
| --------------- | -------------- | ------------------------------------------ | ------- |
| `pr.yml`        | PR             | Pull requests to `main`                    | `build` |
| `main.yml`      | Main           | Pushes to `main`                           | `build` |
| `scheduled.yml` | Scheduled      | Twice a week on `main`, or manually on any branch (*Run workflow* in the Actions tab) | `build` |

The scheduled run catches breakage that comes from outside the repository (runner image updates,
package mirrors) during weeks without commits.

## Naming

Each level of a check name tells one thing:

```
PR / build / lnx-gcc-release
│    │       └── job name in the job file: the dev.py configuration
│    └── job id in the trigger file: the kind of check
└── trigger file name: when and why it ran
```

Keep these names short: GitHub truncates long check names in pull requests.

Branch protection rules match required checks by these names. Update them in the repository
settings when renaming a trigger, a job or a configuration, or pull requests will wait for checks
that never run.

## Job files

Job files are prefixed with `_`, as GitHub doesn't allow subdirectories under
`.github/workflows/`: the prefix groups them and sets them apart from trigger files.

### `_build.yml`

Takes `configs`, a JSON list of [dev.py configurations](../README.md#devpy-commands), and runs
one job per configuration: setup, `cmake --preset`, `cmake --build --preset` and the unit tests
with `ctest --preset`.

The runner comes from the configuration's prefix, as configurations are named
`<os>-<compiler>-<variant>`: `win-` runs on `windows-2022`, anything else on `ubuntu-26.04`. Windows
stays on `windows-2022` because the profiles need Visual Studio 2022 (msvc 194), and newer images
ship Visual Studio 2026 instead.

## Shared steps

`.github/actions/setup` is a composite action holding the steps every job needs before using
CMake:

1. Install system packages (Linux).
2. `dev.py setup`, then add `.venv` to the `PATH` for the following steps.
3. Restore the Conan cache, `dev.py install <config>`, then prune and save the cache when it
   changed.

The Conan cache key combines the runner image, the configuration and a hash of the files that
change the binaries (`requirements.txt`, `conan.lock`, `conanfile.py`, `profiles/`).

Job files share whole jobs, while composite actions share steps between different jobs. A
composite action must be a directory containing an `action.yml` file, so it is referenced with
`uses: ./.github/actions/setup`.

## Adding to the setup

- **Configuration to an existing trigger:** add it to the `configs` list of that trigger file.
  A new OS prefix also needs a runner in `_build.yml`.
- **New trigger** (e.g. a nightly run): add a trigger file calling the existing job files with the
  configurations it needs.
- **New kind of check** (e.g. coverage, clang-tidy, sanitizers): add a `_<check>.yml` job file
  using the setup action, then call it from the triggers that should run it. A sanitizer
  configuration that only differs in its CMake preset can go through `_build.yml` instead.
- **Secrets:** job files don't receive them automatically. Pass them from the trigger with
  `secrets: inherit`.
