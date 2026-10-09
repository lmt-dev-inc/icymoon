"""`dev.py setup`: Create .venv and install build tools."""
import os
import shutil
import subprocess
import sys
from pathlib import Path

from devtools.common import (
    REQUIREMENTS, VENV_CONAN, VENV_DIR, VENV_PYTHON, run, is_venv_active,
)

# Components needed by the msvc and clang-cl profiles
VS_COMPONENTS = [
    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
    "Microsoft.VisualStudio.Component.VC.Llvm.Clang",
]
# What to install to get them. The workload's recommended components include the Windows SDK.
VS_INSTALL_COMPONENTS = [
    "Microsoft.VisualStudio.Workload.VCTools",
    "Microsoft.VisualStudio.Component.VC.Llvm.Clang",
    "Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset",
]


def register(subparsers):
    parser = subparsers.add_parser(
        "setup", help="create .venv and install build tools (conan, cmake)"
    )
    parser.add_argument(
        "--recreate", action="store_true", help="delete and recreate .venv"
    )
    parser.add_argument(
        "--no-toolchain",
        action="store_true",
        help="on Windows, don't check or install the Visual Studio Build Tools (MSVC and clang-cl)",
    )
    parser.set_defaults(func=execute)


def execute(args):
    if args.recreate and is_venv_active():
        sys.exit(
            "Cannot recreate .venv while running from it.\n"
            "Run `deactivate` first, then run `python3 dev.py setup --recreate` again."
        )

    if args.recreate and VENV_DIR.exists():
        print(f"Removing {VENV_DIR}")
        shutil.rmtree(VENV_DIR)

    # Check for the interpreter rather than the folder: a broken or partial
    # venv folder should be recreated
    if not VENV_PYTHON.exists():
        _create_venv()
    else:
        print(f"Using existing virtual environment: {VENV_DIR}")

    run([VENV_PYTHON, "-m", "pip", "install", "--upgrade", "pip"])
    run([VENV_PYTHON, "-m", "pip", "install", "-r", REQUIREMENTS])

    if os.name == "nt" and not args.no_toolchain:
        _ensure_toolchain_installed()
    run([VENV_CONAN, "profile", "detect", "--exist-ok"])

    activate = (
        r".venv\Scripts\activate" if os.name == "nt" else "source .venv/bin/activate"
    )
    print(
        "\nSetup complete.\n"
        "To use conan and cmake from a terminal, activate the virtual environment:\n"
        f"    {activate}\n"
    )


def _create_venv():
    try:
        run([sys.executable, "-m", "venv", VENV_DIR])
    except subprocess.CalledProcessError:
        # Remove the partially created venv so the next run starts clean
        shutil.rmtree(VENV_DIR, ignore_errors=True)
        if sys.platform.startswith("linux"):
            version = f"{sys.version_info.major}.{sys.version_info.minor}"
            print(
                "\nFailed to create the virtual environment.\n"
                "On Debian/Ubuntu, the venv module is packaged separately. Install it with:\n"
                f"    sudo apt install python3-venv  (or python{version}-venv)\n",
                file=sys.stderr,
            )
        raise


def _ensure_toolchain_installed():
    """Install the Visual Studio Build Tools (compilers only, no IDE) if needed.

    MSVC can't be installed by pip or Conan: its license doesn't allow redistributing it.
    Installing or modifying Build Tools asks for administrator rights.
    """
    install_path = __run_vswhere(VS_COMPONENTS)
    if install_path:
        print(f"Using Visual Studio Build Tools: {install_path}")
    else:
        existing_path = __run_vswhere([])
        add_args = [arg for component in VS_INSTALL_COMPONENTS for arg in ("--add", component)]
        add_args.append("--includeRecommended")
        if existing_path:
            # winget can't add components to an existing installation: the VS installer can
            installer = _find_vs_installer_file("setup.exe")
            if not installer:
                sys.exit(
                    f"Visual Studio Installer not found: add the following components to {existing_path} "
                    "with it, then run setup again:\n    " + "\n    ".join(VS_INSTALL_COMPONENTS)
                )
            print(f"Adding the missing C++ components to {existing_path}")
            run([installer, "modify", "--installPath", existing_path, *add_args, "--passive", "--norestart"])
        else:
            if not shutil.which("winget"):
                sys.exit(
                    "winget not found: install the Visual Studio Build Tools with the "
                    "following components, then run setup again:\n    "
                    + "\n    ".join(VS_INSTALL_COMPONENTS)
                )
            run([
                "winget", "install", "--exact", "--id", "Microsoft.VisualStudio.2022.BuildTools",
                "--accept-package-agreements", "--accept-source-agreements",
                "--override", " ".join(["--wait", "--passive", "--norestart", *add_args]),
            ])
        install_path = __run_vswhere(VS_COMPONENTS)
        if not install_path:
            sys.exit("Visual Studio Build Tools installation failed or is incomplete.")

    # Needed to generate the test coverage reports of the win-clang-coverage configuration
    llvm_bin = Path(install_path) / "VC" / "Tools" / "Llvm" / "x64" / "bin"
    for tool in ("clang-cl", "llvm-profdata", "llvm-cov"):
        if not (llvm_bin / f"{tool}.exe").exists():
            print(f"Warning: {tool}.exe not found in {llvm_bin}", file=sys.stderr)


def _find_vs_installer_file(name: str) -> Path | None:
    """A file of the Visual Studio Installer, e.g. vswhere.exe.

    The installer always lives in "%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer",
    wherever Visual Studio itself is installed. Same lookup as Conan's own vswhere detection.
    """
    program_files = os.environ.get("ProgramFiles(x86)") or os.environ.get("ProgramFiles")
    if program_files:
        path = Path(program_files) / "Microsoft Visual Studio" / "Installer" / name
        if path.is_file():
            return path
    return None


def __run_vswhere(components: list[str]) -> str | None:
    """Installation path of the latest Visual Studio product that has all the given components."""
    # vswhere may also be installed separately, e.g. by Chocolatey
    vswhere = _find_vs_installer_file("vswhere.exe") or shutil.which("vswhere")
    if not vswhere:
        return None
    requires = ["-requires", *components] if components else []
    result = subprocess.run(
        [vswhere, "-latest", "-products", "*", *requires, "-property", "installationPath"],
        stdout=subprocess.PIPE, text=True, check=True,
    )
    return result.stdout.strip() or None
