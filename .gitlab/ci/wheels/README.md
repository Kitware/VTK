# Python for Wheels

All macOS and Windows jobs that need Python (wheel jobs and the general CI
venv used for testing) install CPython from `python-build-standalone` via `uv`
at CI time (see `.gitlab/ci/python_macos.sh` and
`.gitlab/ci/python_windows.ps1`), including free-threaded builds. Those builds
are relocatable by construction (the official python.org installers are not
and leave registry rubble around), so this needs no manual prep or upload.

The exact CPython patch version isn't pinned explicitly: `uv` resolves each
job's `PYTHON_VERSION_SUFFIX` (e.g. `3.10`, `3.14t`) to the newest patch it
knows about, which is fixed by the pinned `uv` version in `.gitlab/ci/uv.sh`
and `.gitlab/ci/uv.ps1` -- bump those to pick up new CPython patch releases.
