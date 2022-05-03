# Getting Started Using Linux

## Table of Contents

1. [Introduction](#introduction)
2. [Tools Needed](#tools-needed)
3. [Build VTK](#build-vtk)
    1. [Using the VTK master](#using-the-vtk-master)
    2. [Using a VTK release](#using-a-vtk-release)
4. [Verification](#verification)

## Introduction

These instructions will will lead you step by step through the process of setting up VTK in your home folder.

The instructions are based on Debian versions of Linux implementations. However work-arounds for other implementations are easily achieved.

After completing these instructions, you will have a basic VTK build with Python wrappings. From this base you can then add more options and build settings as outlined in the full documentation [Building VTK](<./build.md>).

We are assuming that you are working in your home folder and the directory structure in your home folder will be:

``` text
|-- dev
  |-- vtk
    |-- build  - where vtk will be built
    |-- src    - the vtk source files
  |-- my-tests - a place to put some test files
```

This folder structure assumes that the VTK master will be checked out. If you use a VTK release instead of the master, `src` will be replaced with the release name.

## Tools Needed

Make sure we have the tools needed to build VTK, also some Python tools:

``` bash
sudo apt install build-essential cmake mesa-common-dev mesa-utils freeglut3-dev python3-dev python3-venv git-core ninja-build
```

The Python tools are optional but may be needed if you decide to use a virtual environment in future.

We will use `ninja` as a replacement for `make`.

For CMake, the latest version is available from [CMake's download page][cmake-download], otherwise:

``` bash
sudo apt-get install cmake cmake-gui
```

These instructions are using the system Python. If you use a different Python implementation or a virtual environment make sure the environment you use is activated.

## Build VTK

Here we create the folder structure, get the VTK source and build it.

### Using the VTK master

``` bash
cd ~
mkdir -p ~/dev/{vtk/{src,build},my-tests}
cd ~/dev/vtk/
git clone --recursive https://gitlab.kitware.com/vtk/vtk.git src
cd ~/dev/vtk/build
cmake ../src -DVTK_WRAP_PYTHON=ON -GNinja
# Build using the generator specified in cmake.
cmake --build .
```

### Using a VTK release

We recommend using the latest release.

``` bash
cd ~
mkdir -p ~/dev/{vtk/build,my-tests}
```

Now download the VTK source from [VTK Releases][vtk-download], untar it into `~/dev/vtk/`. The resultant folder will have a name similar to `VTK-x.y.z` e.g. `VTK-9.2.0`. Remember to substitute `VTK-x.y.z` with the untarred folder name in the following steps.

``` bash
cd ~/dev/vtk/build
cmake ../VTK-x.y.z -DVTK_WRAP_PYTHON=ON -GNinja
# Build using the generator specified in cmake.
cmake --build .
```

## Verification

Download a C++ and Python example and verify that your build of VTK works on them.

Go to the [C++ CylinderExample][cpp-cylinder-example] and download the tarball, extract it in to `~/dev/my-tests`

In the same folder, `~/dev/my-tests`, create a file called `CylinderExample.py` and mark it executable. Go to [Python CylinderExample][python-cylinder-example] and copy it into your `CylinderExample.py` and save it.

Now test using `vtkpython`, `python3` and then build and test the C++ version:

``` bash
cd ~/dev/my-tests
# Set your environment.
export VTK_DIR=$HOME/dev/vtk/build
export PYTHONPATH=~/dev/vtk/build/lib/python3.9/site-packages:PYTHONPATH
alias vtkpython=~/dev/vtk/build/bin/vtkpython
# Test vtkpython.
vtkpython CylinderExample.py
# Test Python.
python3 CylinderExample.py
# Build the C++ version.
cd ~/dev/my-tests/CylinderExample/build
cmake .. -GNinja
# Build using the generator specified in cmake.
cmake --build .
# Test the C++ build.
./CylinderExample
```

Note:

- `PYTHONPATH` is only needed if you are using the python executable e.g. `python3`. It is not needed if `vtkpython` is used.
- With respect to `PYTHONPATH` you may have a different Python version so check your path.
- `VTK_DIR` allows cmake to find where VTK is. When you develop your own code you can just use:

  ``` text
  -DVTK_DIR="<path to my vtk build>"
  ```

  as a cmake option.

[cmake-download]: https://cmake.org/download/
[cpp-cylinder-example]: https://kitware.github.io/vtk-examples/site/Cxx/GeometricObjects/CylinderExample/
[ninja]: https://ninja-build.org/
[python-cylinder-example]: https://kitware.github.io/vtk-examples/site/Python/GeometricObjects/CylinderExample/
[vtk-download]: https://vtk.org/download/
