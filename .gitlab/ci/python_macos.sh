#!/bin/sh
# Sets up a Python for macOS CI jobs and exports `PYTHON_PREFIX`. This script
# must be sourced. Python is installed via `uv` from python-build-standalone:
# those builds are relocatable out of the box (the official python.org
# installers are not; see `.gitlab/ci/wheels/README.md`) and are the only
# source of free-threaded macOS interpreters.
#
# `$PYTHON_VERSION_SUFFIX` (e.g. "3.10", "3.14t") is passed to `uv` as a bare
# minor-version request rather than an exact patch pin: `uv` resolves it to
# the newest matching patch it knows about, from a table compiled into the
# `uv` binary itself. That resolution is exactly reproducible as long as
# `.gitlab/ci/uv.sh`'s pinned `uv` version doesn't change, so new CPython
# patch releases are picked up by bumping that one pin instead of updating a
# per-version table here.

set -e

.gitlab/ci/uv.sh
export UV_CACHE_DIR="$GIT_CLONE_PATH/.gitlab/uv-cache"
export UV_PYTHON_INSTALL_DIR="$GIT_CLONE_PATH/.gitlab/uv-python"
# `--no-bin` keeps executables out of `~/.local/bin`; the runners are shared
# machines and jobs must not leave state outside the workspace.
.gitlab/uv python install --no-bin "cpython-$PYTHON_VERSION_SUFFIX"
PYTHON_PREFIX="$( dirname "$( dirname "$( .gitlab/uv python find --managed-python "cpython-$PYTHON_VERSION_SUFFIX" )" )" )"
export PYTHON_PREFIX
