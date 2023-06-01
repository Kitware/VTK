# Building

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

VTK supports all of the common generators supported by CMake. The Ninja,
Makefiles, and Visual Studio generators are the most well-tested however.

Note that VTK does not support in-source builds, so you must have a build tree
that is not the source tree.

## Obtaining the source

To obtain VTK's sources locally, clone this repository using
[Git][git].

```sh
git clone --recursive https://gitlab.kitware.com/vtk/vtk.git
```

## Prerequisites

VTK only requires a few packages in order to build in general, however
specific features may require additional packages to be provided to VTK's
build configuration.

Required:

  * [CMake][cmake]
    - Version 3.12 or newer, however, the latest version is always recommended.
    If the system package management utilities do not offer cmake or if the offered version is too old
    Precompiled binaries available on [CMake's download page][cmake-download].

    ```{tip}
    If you are using cmake through the system's package management it is highly recommended install
    `ccmake` (package `cmake-curses-gui` on Ubuntu/Debian) that allows to interactively set building options.
    ```

  * Supported compiler
    - GCC 4.8 or newer
    - Clang 3.3 or newer
    - Apple Clang 7.0 (from Xcode 7.2.1) or newer
    - Microsoft Visual Studio 2015 or newer
    - Intel 14.0 or newer

### Optional Additions

- **ffmpeg**
  When the ability to write `.avi` files is desired, and writing these files is
  not supported by the OS, VTK can use the ffmpeg library. This is generally
  true for Unix-like operating systems. Source code for ffmpeg can be obtained
  from [the website][ffmpeg].

- **MPI**
  To run VTK in parallel, an [MPI][mpi] implementation is required. If an MPI
  implementation that exploits special interconnect hardware is provided on your
  system, we suggest using it for optimal performance. Otherwise, on Linux/Mac,
  we suggest either [OpenMPI][openmpi] or [MPICH][mpich]. On Windows, [Microsoft
  MPI][msmpi] is required.

- **Python**
  In order to use scripting, [Python][python] is required. The minimum supported version is **3.4**.
  The instructions are using the system Python. On Ubuntu/Debian the required package is
  `python3-dev`. If you use a different Python
  implementation or a virtual environment make sure the environment you use is
  activated. On Ubuntu/Debian the required package for creating virtual environments is
  `python3-venv`.

- **Qt5**
  VTK uses Qt as its GUI library (if the relevant modules are enabled).
  Precompiled binaries are available on [Qt's website][qt-download].
  Note that on Windows, the compiler used for building VTK must match the
  compiler version used to build Qt. Version **5.9** or newer is required.

- **OSMesa**
  Off-screen Mesa can be used as a software-renderer for running VTK on a server
  without hardware OpenGL acceleration. This is usually available in system
  packages on Linux. For example, the `libosmesa6-dev` package on Debian and
  Ubuntu. However, for older machines, building a newer version of Mesa is
  likely necessary for bug fixes and support. Its source and build instructions
  can be found on [its website][mesa].

## Creating the Build Environment

### Linux (Ubuntu/Debian)

Install  the following packages:

```bash
$ sudo apt install \
build-essential \
cmake \
mesa-common-dev \
mesa-utils \
freeglut3-dev \
ninja-build
```

```{tip}
`ninja` is a speedy replacement for `make`, highly recommended.
```

### Windows

  * [Visual Studio Community Edition][visual-studio]

  * Use "x64 Native Tools Command Prompt" for the installed Visual Studio
    version to configure with CMake and to build with ninja.

  * Get [ninja][ninja]. Unzip the binary and put it in `PATH`. Note that newer
    Visual Studio releases come with a version of `ninja` already and should
    already exist in `PATH` within the command prompt.

## Configure and Building

In order to build, CMake requires two steps, configure and build. VTK itself
does not support what are known as in-source builds, so the first step is to
create a build directory.

```sh
mkdir -p vtk/build
cd vtk/build
ccmake -GNinja ../path/to/vtk/source # -GNinja may be skipped to use the default generator for each platform
```

CMake's GUI has input entries for the build directory and the generator
already. Note that on Windows, the GUI must be launched from a "Native Tools
Command Prompt" available with Visual Studio in the start menu.

```{admonition} **Missing dependencies**
:class: warning

CMake may not find all dependencies automatically in all cases. The steps
needed to find any given package depends on the package itself. For general
assistance, please see the documentation for
[`find_package`'s search procedure][cmake-find_package-search] and
[the relevant Find module][cmake-modules-find] (as available).
```

To build vtk run:

```sh
cmake --build .
```

```{tip}
Different features can be enabled/disabled by setting the [Build Settings](build_settings.md) during the configure stage.
```

[cmake]: https://cmake.org
[cmake-download]: https://cmake.org/download
[cmake-find_package-search]: https://cmake.org/cmake/help/latest/command/find_package.html#search-procedure
[cmake-modules-find]: https://cmake.org/cmake/help/latest/manual/cmake-modules.7.html#find-modules
[ffmpeg]: https://ffmpeg.org
[git]: https://git-scm.org
[mesa]: https://www.mesa3d.org
[mpi]: https://www.mcs.anl.gov/research/projects/mpi
[mpich]: https://www.mpich.org
[msmpi]: https://docs.microsoft.com/en-us/message-passing-interface/microsoft-mpi
[ninja]: https://ninja-build.org
[openmpi]: https://www.open-mpi.org
[python]: https://python.org
[qt]: https://qt.io
[qt-download]: https://download.qt.io/official_releases/qt
[visual-studio]: https://visualstudio.microsoft.com/vs
