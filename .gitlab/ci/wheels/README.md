# Python for Wheels

This directory contains scripts to create artifacts for CI to use when building
wheels on Windows. The official installers end up having behaviors that are
not suitable for CI purposes and therefore cannot be created in CI either, so
it is a manual process.

## macOS

All macOS jobs that need Python (wheel jobs and the general CI venv used for
testing) install CPython from `python-build-standalone` via `uv` at CI time
(see `.gitlab/ci/python_macos.sh`), including free-threaded builds. Those
builds are relocatable by construction (the official python.org installers
are not, which is why macOS used to need the same kind of manual prep/upload
that Windows still does below), so this needs no manual prep or upload. The
exact CPython patch version isn't pinned explicitly: `uv` resolves each job's
`PYTHON_VERSION_SUFFIX` (e.g. `3.10`, `3.14t`) to the newest patch it knows
about, which is fixed by the pinned `uv` version in `.gitlab/ci/uv.sh` — bump
that to pick up new CPython patch releases.

## Windows

The official installers leave registry rubble around. The
`prep_python_windows.ps1` script creates a zip from the official installer by
installing into a local directory and then packing it up. Once the `.zip` is
created, move it to a Unix machine and unpack it into a directory with the
basename of the `.zip` file. Then repack the `.zip` to include this directory.
Also be sure to use the Windows Add/Remove Programs process to remove the
installation made during this process.

### Free-threaded builds

To create a free-threaded Python distribution (Python 3.13+), pass the
`-freethreading` flag:

```powershell
.\prep_python_windows.ps1 -version 3.13.1 -freethreading
```

This will produce a zip with `t` in the name (e.g.,
`python-3.13.1t-windows-x86_64.zip`).

## Uploading (Windows)

Upload the `prep_python_windows.ps1` output using [the upload
process][upload-files] to
`https://www.paraview.org/files/dependencies/python-for-wheels`.

[upload-files]: https://gitlab.kitware.com/utils/git-workflow/-/wikis/File-hosting

## Updating CI (Windows)

Update the `.gitlab/ci/download_wheel_python.cmake` file with the version and
hashes as needed. When adding a new minor version, it is recommended to also
update any patch releases as available.
