# Set up the tools and environment of the Linux jobs. This is sourced (and not
# run) so that the environment changes persist.

.gitlab/ci/cmake.sh
if echo "$CMAKE_CONFIGURATION" | grep -q "mindeps"; then
    env CMAKE_SUFFIX=-mindeps .gitlab/ci/cmake-mindeps.sh
fi
.gitlab/ci/ninja.sh
export PATH="$PWD/.gitlab:$PWD/.gitlab/cmake/bin:$PATH"

# Download Qt
cmake -P .gitlab/ci/download_qt6.cmake
export CMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH:$PWD/.gitlab/qt6"

cmake --version
ninja --version

$LAUNCHER cmake -P "$PWD/.gitlab/ci/fetch_tags.cmake"

# Load any modules that may be necessary.
if [ -n "$MODULES" ]; then
    . /etc/profile.d/modules.sh
    module load $MODULES
fi

# Load a venv
if [ -n "$VENV_ROOT" ]; then
    . "$VENV_ROOT/bin/activate"
    # activate does not set PYTHONPATH which is needed for vtkpython to pickup
    # packages installed in a separate venv.
    # https://gitlab.kitware.com/vtk/vtk/-/issues/19539
    export PYTHONPATH="$VENV_ROOT/lib/$SITE_PACKAGES_DIR/site-packages"
fi

# Enable the Intel compiler if required.
if [ "$CC" = "icx" ]; then
    . /opt/intel/oneapi/setvars.sh || true
fi
