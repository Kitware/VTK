# Getting Started Using Linux

## Contents

1. [Introduction](#introduction)
2. [Preliminary Steps](#preliminary-steps)
    1. [Update your installation](#update-your-installation)
    2. [Setup Python](#setup-python)
3. [Create local source and build folders for VTK](#create-local-source-and-build-folders-for-vtk)
4. [Build VTK](#build-vtk)
5. [Testing](#testing)
6. [Additional Comments](#additional-comments)

## Introduction

These instructions will will lead you step by step through the process of setting up VTK in your home folder.

After completing these instructions, you will have a basic VTK build with Python wrappings. From this base you can then add more options and build settings as outlined in the full documentation [Building VTK](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Documentation/dev/build.md).

We are assuming that you are working in your home folder and the directory structure in your home folder will be:

``` text
|-- Kitware
  |-- build
  |-- src
  |-- test

```

There will be a `VTK` folder in each of `build`, `src`, `test`. The advantage of this structure is a clear functional separation between sources and builds and it also allows adding future sources and builds such as for **ParaView** and the **vtk-examples** in their own sub-folders.

## Preliminary Steps

You will need **`sudo`** rights to use apt.

### Update your installation

``` bash
sudo apt-get update
sudo apt-get upgrade
```

Make sure we have the tools needed to build VTK:

``` bash
sudo apt-get install build-essential libncurses5-dev libxext-dev mesa-common-dev mesa-utils freeglut3-dev python3-dev python3-venv git-core gitk git-gui ninja-build
```

In your environment e.g. `~/.bashrc` set:

``` bash
alias ninja=ninja-build
```

For CMake, it is best to download and install CMake from [Get the Software](https://cmake.org/download/), it's a good idea to make sure `cmake-gui` is installed. Note that, if you are building `cmake-gui`, Qt needs to be installed. This is the preferred option as you will get the latest version.

The alternative is:

``` bash
sudo apt-get install cmake cmake-gui
```

Be aware this version may be an older release.

### Setup Python

We will use a virtual environment for VTK, let's call it `VTK`.

If you are already in a virtual environment `deactivate` to make sure the correct version of python is being used.

Create the virtual environment:

``` bash
deactivate
python3 -m venv $HOME/venv/VTK
```

Activate and install the necessary components for VTK:

``` bash
source $HOME/venv/VTK/bin/activate
python -m pip install --upgrade pip
pip install wheel
pip install Sphinx scipy numpy matplotlib pytz tzdata setuptools
deactivate
```

In future, if you want to upgrade packages:

``` bash
source $HOME/venv/VTK/bin/activate
python -m pip install --upgrade pip
pip install --upgrade Sphinx scipy numpy matplotlib pytz tzdata setuptools wheel
deactivate
```

At this stage you have a virtual environment for VTK, test it:

``` bash
source $HOME/venv/VTK/bin/activate
python -V
deactivate
```

## Create local source and build folders for VTK

The source for building VTK will be in `~/Kitware/src/VTK`, the build for VTK will be in `~/Kitware/build/VTK` with any tests in `~/Kitware/tests/VTK`.

``` bash
cd ~
mkdir -p ~/Kitware/{src/VTK,build/VTK,tests/VTK}
```

## Build VTK

Check out the master using git:

```bash
cd ~/Kitware/src
git clone --recursive https://gitlab.kitware.com/vtk/vtk.git VTK
```

**Note:** *An alternative is to download the [VTK Latest Release](https://vtk.org/download/), unpack it and move/copy the contents into `~/Kitware/src/VTK`.*

Let's just do a minimal build, you can "build" on this later.
For more information see [Building VTK](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Documentation/dev/build.md).

So for CMake we need the following settings:

* `BUILD_SHARED_LIBS`: `ON` - should be `ON` by default
* `VTK_ALL_NEW_OBJECT_FACTORY`: `ON`
* `VTK_ENABLE_WRAPPING`: `ON` - should be `ON` by default
* `VTK_WRAP_PYTHON`: `ON`

We will use `cmake-gui` to do the VTK configuration.

**Important:** *Before even configuring VTK for a build using CMake, make sure your Python virtual environment is activated.*

``` bash
cd ~/Kitware/build/VTK
source $HOME/venv/VTK/bin/activate
cmake-gui ~/Kitware/src/VTK
```

Press `Configure`, specifying `Ninja` as the generator and then set the above settings to `ON`. Use `Search` to find the settings.

Once you have set everything, press `Generate`.

Exit and run `ninja`:

``` bash
ninja
```

**Note:** *If the build fails. Run cmake-gui again and set `VTK_PYTHON_OPTIONAL_LINK` to `OFF` then Configure, Generate, exit and run ninja again.*

## Testing

Go to [CylinderExample](https://kitware.github.io/vtk-examples/site/Cxx/GeometricObjects/CylinderExample/) and download the tarball, extract it, and move it into `Kitware/test/VTK`

Create a file called `CylinderExample.py` in `Kitware/test/VTK`, mark it executable (if you want) and open it in a text editor. Go to the python version of [CylinderExample](https://kitware.github.io/vtk-examples/site/Python/GeometricObjects/CylinderExample/) and copy it into your `CylinderExample.py` and save it.

We need to pick up where the vtk build is and where the lib is:

``` bash
export VTK_DIR=$HOME/Kitware/build/VTK
source $VTK_DIR/unix_path.sh
alias vtkpython=$VTK_DIR/bin/vtkpython
```

Now go to the Testing folder and build the test:

``` bash
cd ~/Kitware/test/VTK/CylinderExample/build
cmake-gui ..
```

Press `Configure`, specifying `Ninja` as the generator then press `Generate` and exit, then:

``` bash
ninja
./CylinderExample
```

For Python, you can use either `python` or `vtkpython`:

``` bash
cd ~/Kitware/test/VTK
python CylinderExample.py
vtkpython CylinderExample.py
```

If the CylinderExample is executable:

``` bash
./CylinderExample.py
```

## Additional Comments

You **must** set the environment as follows:

``` bash
source $HOME/venv/VTK/bin/activate
export VTK_DIR=$HOME/Kitware/build/VTK
source $VTK_DIR/unix_path.sh
alias vtkpython=$VTK_DIR/bin/vtkpython
```

This can be put into a bash script that you can source.

Here is an example:

``` bash
# Set the correct environment when using VTK
#
source $HOME/venv/VTK/bin/activate
export VTK_DIR=$HOME/Kitware/build/VTK
if [ -f $VTK_DIR/unix_path.sh ]
then
  source $VTK_DIR/unix_path.sh
else
  # Note: x86_64-linux-gnu and python3.9 depend on your implementation
  #       and may be different from the values here.
  export LD_LIBRARY_PATH=$VTK_DIR/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
  export PYTHONPATH=$VTK_DIR/lib/x86_64-linux-gnu/python3.10/site-packages:$PYTHONPATH
fi
alias vtkpython=$VTK_DIR/bin/vtkpython
```

Copy this into a file called `VTK.sh` and make it executable. A good place to put this would be in `~/Kitware`. Then you can just:

``` bash
source ~/Kitware/VTK.sh
```

Thereby setting your VTK environment for whenever you need to:

* build VTK
* build your own code
* run VTK Python scripts

If you use an IDE like PyCharm, set the Virtual Environment to the existing VTK Virtual Environment e.g. `~/venv/VTK/bin/python` and add the path to the VTK site packages in the interpreter paths, e.g.: `~/Kitware/build/VTK/lib/x86_64-linux-gnu/python3.9/site-packages`
