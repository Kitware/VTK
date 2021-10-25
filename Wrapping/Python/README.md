# The vtkmodules package and vtkpython

## vtkmodules

The 'vtkmodules' directory contains python files (and .in template files)
that form part of the vtkmodules package.  When VTK is built, this package
is created in the Python site-packages directory in the build tree.  For
linux and macOS, the build location of this directory is here:

    lib/pythonx.y/site-packages

On Windows, it is here:

    bin/Lib/site-packages

In order to use the vtkmodules package from the build tree, this
site-packages directory must be in Python's path, and all the VTK
shared libraries (and their dependencies) must be in the system path.
The easiest way to ensure this is to run the vtkpython executable
that is described below, since it sets up those paths internally when
it runs.  To use the regular python executable, you can set up the
environment with the following script at the root of the build tree:

    source ./unix_path.sh

Or, on Windows, use one of these:

    windows_path.bat
    windows_path.<config>.bat

Note that with Python 3.8 and later, the `os.add_dll_directory()` method
can be used on Windows to add to the DLL path.  The vtkmodules package
will call this automatically, but if you have trouble importing the VTK
modules, you can try calling it yourself at the beginning of your program
before any VTK modules are imported.

The 'vtk.py' module is obsolete and is only provided for backwards
compatibility.  When you `import vtk`, this module actually provides
you with the 'vtkmodules.all' module via some internal trickery:

    >>> import vtk
    >>> vtk.__name__
    'vtkmodules.all'

## vtkpython

When executing Python scripts in the VTK build tree, the necessary paths
are usually not set since that is done at the time of installation.
The vtkpython executable includes the necessary paths internally, and can
be used to run the scripts without setting any environment variables.

The only time it should be necessary to set the PATH is when VTK has been
built with dependencies (shared libraries, specifically) which are not
anywhere in the system path.  Note that on Windows, from Python 3.9
onwards, DLL paths must be added by calling `os.add_dll_directory()`
within Python rather than by setting PATH.

The vtkpython executable is created from `vtkPythonAppInit.cxx` and, on
Windows, `vtkpython.rc`.  Most of the implementation of this executable
resides in the Utilities/PythonInterpreter directory.
