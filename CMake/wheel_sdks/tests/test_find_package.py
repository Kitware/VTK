from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path
from typing import Literal, overload

import pytest
import virtualenv as _virtualenv  # type: ignore[import-untyped]

DIR = Path(__file__).parent.resolve()
BASE = DIR / "packages" / "find_package"


class VEnv:
    def __init__(self, env_dir: Path, *, wheelhouse: Path | None = None) -> None:
        cmd = [str(env_dir), "--no-setuptools", "--no-wheel", "--activators", ""]
        result = _virtualenv.cli_run(cmd, setup_logging=False)
        self.wheelhouse = wheelhouse
        self.executable = Path(result.creator.exe)
        self.env_dir = env_dir.resolve()
        self.platlib = Path(
            self.execute("import sysconfig; print(sysconfig.get_path('platlib'))")
        )
        self.purelib = Path(
            self.execute("import sysconfig; print(sysconfig.get_path('purelib'))")
        )

    @overload
    def run(self, *args: str, capture: Literal[True]) -> str: ...

    @overload
    def run(self, *args: str, capture: Literal[False] = ...) -> None: ...

    def run(self, *args: str, capture: bool = False) -> str | None:
        __tracebackhide__ = True
        env = os.environ.copy()
        paths = {str(self.executable.parent)}
        env["PATH"] = os.pathsep.join([*paths, env["PATH"]])
        env["VIRTUAL_ENV"] = str(self.env_dir)
        env["PIP_DISABLE_PIP_VERSION_CHECK"] = "ON"
        if self.wheelhouse is not None:
            env["PIP_NO_INDEX"] = "ON"
            env["PIP_FIND_LINKS"] = str(self.wheelhouse)

        str_args = [os.fspath(a) for a in args]

        # Windows does not make a python shortcut in venv
        if str_args[0] in {"python", "python3"}:
            str_args[0] = str(self.executable)

        if capture:
            result = subprocess.run(
                str_args,
                check=False,
                capture_output=True,
                text=True,
                env=env,
            )
            if result.returncode != 0:
                print(result.stdout, file=sys.stdout)
                print(result.stderr, file=sys.stderr)
                print("FAILED RUN:", *str_args, file=sys.stderr)
                raise SystemExit(result.returncode)
            return result.stdout.strip()

        result_bytes = subprocess.run(
            str_args,
            check=False,
            env=env,
        )
        if result_bytes.returncode != 0:
            print("FAILED RUN:", *str_args, file=sys.stderr)
            raise SystemExit(result_bytes.returncode)
        return None

    def execute(self, command: str) -> str:
        return self.run(str(self.executable), "-c", command, capture=True)

    def module(self, *args: str) -> None:
        return self.run(str(self.executable), "-m", *args)

    def install(self, *args: str, isolated: bool = True) -> None:
        isolated_flags = "" if isolated else ["--no-build-isolation"]
        self.module("pip", "install", *isolated_flags, *args)


@pytest.fixture()
def virtualenv(tmp_path: Path) -> VEnv:
    path = tmp_path / "venv"
    return VEnv(path)


def test_find_package(virtualenv: VEnv):
    virtualenv.run(
        "python", "-m", "pip", "install", "--find-links", os.curdir, str(BASE)
    )
