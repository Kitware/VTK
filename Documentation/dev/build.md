# Building VTK

This page describes how to build and install VTK. It covers building for
development, on both Unix-type systems (Linux, HP-UX, Solaris, macOS), and
Windows. Note that Unix-like environments such as Cygwin and MinGW are not
officially supported. However, patches to fix problems with these platforms
will be considered for inclusion. It is recommended that users which require
VTK to work on these platforms to submit nightly testing results for them.

A full-featured build of VTK depends on several open source tools and libraries
such as Python, Qt, CGNS, HDF5, etc. Some of these are included in the VTK
source itself (e.g., HDF5), while others are expected to be present on the
machine on which VTK is being built (e.g., Python, Qt).

## Obtaining the source

To obtain VTK's sources locally, clone this repository using
[Git][git].

```sh
git clone --recursive https://gitlab.kitware.com/vtk/vtk.git
```

## Building

VTK supports all of the common generators supported by CMake. The Ninja,
Makefiles, and Visual Studio generators are the most well-tested however.

### Prerequisites

VTK only requires a few packages in order to build in general, however
specific features may require additional packages to be provided to VTK's
build configuration.

Required:

  * [CMake][cmake]
    - Version 3.8 or newer, however, the latest version is always recommended
  * Supported compiler
    - GCC 4.8 or newer
    - Clang 4 or newer
    - Xcode 9 or newer
    - Visual Studio 2015 or newer

Optional dependencies:

  * [Python][python]
    - When using Python 2, at least 2.7 is required
    - When using Python 3, at least 3.3 is required
  * [Qt5][qt]
    - Version 5.9 or newer

#### Installing CMake

CMake is a tool that makes cross-platform building simple. On several systems
it will probably be already installed or available through system package
management utilities. If it is not, there are precompiled binaries available on
[CMake's download page][cmake-download].

#### Installing Qt

VTK uses Qt as its GUI library (if the relevant modules are enabled).
Precompiled binaries are available on [Qt's website][qt-download].

Note that on Windows, the compiler used for building VTK must match the
compiler version used to build Qt.

### Optional Additions

#### Download And Install ffmpeg (`.avi`) movie libraries

When the ability to write `.avi` files is desired, and writing these files is
not supported by the OS, VTK can use the ffmpeg library. This is generally
true for Unix-like operating systems. Source code for ffmpeg can be obtained
from [the website][ffmpeg].

#### MPI

To run VTK in parallel, an [MPI][mpi] implementation is required. If an MPI
implementation that exploits special interconnect hardware is provided on your
system, we suggest using it for optimal performance. Otherwise, on Linux/Mac,
we suggest either [OpenMPI][openmpi] or [MPICH][mpich]. On Windows, [Microsoft
MPI][msmpi] is required.

#### Python

In order to use scripting, [Python][python] is required (versions 2.7 and 3.3).

#### OSMesa

Off-screen Mesa can be used as a software-renderer for running VTK on a server
without hardware OpenGL acceleration. This is usually available in system
packages on Linux. For example, the `libosmesa6-dev` package on Debian and
Ubuntu. However, for older machines, building a newer version of Mesa is
likely necessary for bug fixes and support. Its source and build instructions
can be found on [its website][mesa].

## Creating the Build Environment

### Linux (Ubuntu/Debian)

  * `sudo apt install` the following packages:
    - `build-essential`
    - `cmake`
    - `mesa-common-dev`
    - `mesa-utils`
    - `freeglut3-dev`
    - `ninja-build`
      - `ninja` is a speedy replacement for `make`, highly recommended.

### Windows

  * [Visual Studio Community Edition][visual-studio]
  * Use "x64 Native Tools Command Prompt" for the installed Visual Studio
    version to configure with CMake and to build with ninja.
  * Get [ninja][ninja]. Unzip the binary and put it in `PATH`. Note that newer
    Visual Studio releases come with a version of `ninja` already and should
    already exist in `PATH` within the command prompt.

## Building

In order to build, CMake requires two steps, configure and build. VTK itself
does not support what are known as in-source builds, so the first step is to
create a build directory.

```sh
mkdir -p vtk/build
cd vtk/build
ccmake ../path/to/vtk/source # -GNinja may be added to use the Ninja generator
```

CMake's GUI has input entries for the build directory and the generator
already. Note that on Windows, the GUI must be launched from a "Native Tools
Command Prompt" available with Visual Studio in the start menu.

### Build Settings

VTK has a number of settings available for its build. The common variables
to modify include:

  * `BUILD_SHARED_LIBS` (default `ON`): If set, shared libraries will be
    built. This is usually what is wanted.
  * `VTK_USE_CUDA` (default `OFF`): Whether CUDA support will be available or
    not.
  * `VTK_USE_MPI` (default `OFF`): Whether MPI support will be available or
    not.
  * `VTK_WRAP_PYTHON` (default `OFF`): Whether Python support will be
    available or not.
  * `VTK_PYTHON_VERSION` (default `2`): The major version of Python to
    support. Must be either `2` or `3`.

Less common, but variables which may be of interest to some:

  * `VTK_BUILD_EXAMPLES` (default `OFF`): If set, VTK's example code will be
    added as tests to the VTK test suite.
  * `VTK_ENABLE_LOGGING` (default `ON`): If set, enhanced logging will be
    enabled.
  * `VTK_BUILD_TESTING` (default `OFF`): Whether to build tests or not. Valid
    values are `OFF` (no testing), `WANT` (enable tests as possible), and `ON`
    (enable all tests; may error out if features otherwise disabled are
    required by test code).
  * `VTK_ENABLE_KITS` (default `OFF`): requires CMake 3.12+): Compile VTK into
    a smaller set of libraries. Can be useful on platforms where VTK takes a
    long time to launch due to expensive disk access.
  * `VTK_WRAP_JAVA` (default `OFF`): Whether Java support will be available or
    not.

More advanced options:

  * `VTK_BUILD_DOCUMENTATION` (default `OFF`): If set, VTK will build its API
    documentation using Doxygen.
  * `VTK_BUILD_ALL_MODULES` (default `OFF`): If set, VTK will enable all
    modules not disabled by other features.
  * `VTK_USE_EXTERNAL` (default `OFF`): Whether to prefer external third
    party libraries or the versions VTK's source contains.
  * `VTK_VERSIONED_INSTALL` (default `ON`): Whether to add version numbers to
    VTK's include directories and library names in the install tree.
  * `VTK_CUSTOM_LIBRARY_SUFFIX` (default depends on `VTK_VERSIONED_INSTALL`):
    The custom suffix for libraries built by VTK. Defaults to either an empty
    string or `X.Y` where `X` and `Y` are VTK's major and minor version
    components, respectively.
  * `VTK_INSTALL_SDK` (default `ON`): If set, VTK will install its headers,
    CMake API, etc. into its install tree for use.
  * `VTK_RELOCATABLE_INSTALL` (default `ON`): If set, the install tree will be
    relocatable to another path. If unset, the install tree may be tied to the
    build machine with absolute paths, but finding dependencies in
    non-standard locations may require work without passing extra information
    when consuming VTK.
  * `VTK_USE_LARGE_DATA` (default `OFF`; requires `VTK_BUILD_TESTING`):
    Whether to enable tests which use "large" data or not (usually used to
    reduce the amount of data downloading required for the test suite).
  * `VTK_LEGACY_REMOVE` (default `OFF`): If set, VTK will disable legacy,
    deprecated APIs.
  * `VTK_LEGACY_SILENT` (default `OFF`; requires `VTK_LEGACY_REMOVE` to be
    `OFF`): If set, usage of legacy, deprecated APIs will not cause warnings.
  * `VTK_USE_TK` (default `OFF`; requires `VTK_WRAP_PYTHON`): If set, VTK will
    enable Tkinter support for VTK widgets.
  * `VTK_BUILD_COMPILE_TOOLS_ONLY` (default `OFF`): If set, VTK will compile
    just its compile tools for use in a cross-compile build.

The VTK module system provides a number of variables to control modules which
are not otherwise controlled by the other options provided.

  * `VTK_MODULE_USE_EXTERNAL_<name>` (default depends on `VTK_USE_EXTERNAL`):
    Use an external source for the named third-party module rather than the
    copy contained within the VTK source tree.

    > **_WARNING:_**
    >
    > Activating this option within an interactive cmake configuration (i.e. ccmake, cmake-gui)
    > could end up finding libraries in the standard locations rather than copies
    > in non-standard locations.
    >
    > It is recommended to pass the variables necessary to find the intended external package to
    > the first configure to avoid finding unintended copies of the external package.
    > The variables which matter depend on the package being found, but those ending with
    > `_LIBRARY` and `_INCLUDE_DIR` as well as the general CMake `find_package` variables ending
    > with `_DIR` and `_ROOT` are likely candidates.
    >
    > ```
    > Example:
    > ccmake -D HDF5_ROOT:PATH=/home/user/myhdf5 ../vtk/sources
    > ```

  * `VTK_MODULE_ENABLE_<name>` (default `DEFAULT`): Change the build settings
    for the named module. Valid values are those for the module system's build
    settings (see below).
  * `VTK_GROUP_ENABLE_<name>` (default `DEFAULT`): Change the default build
    settings for modules belonging to the named group. Valid values are those
    for the module system's build settings (see below).

For variables which use the module system's build settings, the valid values are as follows:

  * `YES`: Require the module to be built.
  * `WANT`: Build the module if possible.
  * `DEFAULT`: Use the settings by the module's groups and
    `VTK_BUILD_ALL_MODULES`.
  * `DONT_WANT`: Don't build the module unless required as a dependency.
  * `NO`: Do not build the module.

If any `YES` module requires a `NO` module, an error is raised.

#### Mobile devices

VTK supports mobile devices in its build. These are triggered by a top-level
flag which then exposes some settings for a cross-compiled VTK that is
controlled from the top-level build.

iOS builds may be enabled by setting the `VTK_IOS_BUILD` option. The following
settings than affect the iOS build:

  * `IOS_SIMULATOR_ARCHITECTURES`
  * `IOS_DEVICE_ARCHITECTURES`
  * `IOS_DEPLOYMENT_TARGET`
  * `IOS_EMBED_BITCODE`

Android builds may be enabled by setting the `VTK_ANDROID_BUILD` option. The
following settings affect the Android build:

  * `ANDROID_NDK`
  * `ANDROID_NATIVE_API_LEVEL`
  * `ANDROID_ARCH_ABI`

## Building documentation

The following targets are used to build documentation for VTK:

  * `DoxygenDoc` - build the doxygen documentation from VTK's C++ source files.

[cmake]: https://cmake.org
[cmake-download]: https://cmake.org/download
[ffmpeg]: https://ffmpeg.org
[git]: https://git-scm.org
[mesa]: https://www.mesa3d.org
[mpi]: https://www.mcs.anl.gov/research/projects/mpi
[ninja]: https://ninja-build.org
[msmpi]: https://docs.microsoft.com/en-us/message-passing-interface/microsoft-mpi
[mpich]: https://www.mpich.org
[nvpipe]: https://github.com/NVIDIA/NvPipe
[openmpi]: https://www.open-mpi.org
[python]: https://python.org
[qt]: https://qt.io
[qt-download]: https://download.qt.io/official_releases/qt
[visual-studio]: https://visualstudio.microsoft.com/vs
