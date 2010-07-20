================================
VTK-Python Package Documentation
================================
:Contact: vtk-developers@vtk.org

.. contents::


Introduction
^^^^^^^^^^^^

This file documents the VTK-Python modules.

The file `README_WRAP.txt` provides a detailed description of how VTK
objects are accessed and modified through Python.


Installation
^^^^^^^^^^^^

VTK can be built either from inside the source directory (an
in-source build) or in another directory separate from the source
directory (called an out-of-source build).  The VTK source directory
is called `VTK_SOURCE_DIR` and the directory in which VTK is built
is called `VTK_BINARY_DIR`.  Note that `VTK_BINARY_DIR` is identical
to `VTK_SOURCE_DIR` when an in-source build is performed.  Both
these variables are defined in `CMakeCache.txt`.  The following
instructions apply to both in-source or out-of-source builds.  The
variable `VTK_ROOT` used in the following refers to
`VTK_BINARY_DIR`.

There are primarily three ways of using the VTK-Python wrappers and
modules.

  (1) If you are building VTK from source and want to use the
      VTK-Python packages alone, i.e. you don't want to use VTK via
      C++, Tcl, etc., then simply use the `vtkpython` interpreter
      that is built with VTK.  This is a Python interpreter that is
      linked to the relevant VTK libraries and sets the `PYTHONPATH`
      internally to the correct locations.  So, all you need to do
      is use `vtkpython` instead of `python`.  Naturally this will
      work only if you retain your build tree and don't change the
      directories where VTK was built.

  (2) Using the package from the source build without installing it
      system wide and without using `vtkpython`.  This is most
      useful when you build VTK off a CVS checkout and do not want
      to install it system wide and still want to use the vanilla
      Python interpreter.  This is also useful if you are not the
      administrator of the machine you are using/building VTK on.

    Unix/Linux
      Under Unix the way to do this is to set the `LD_LIBRARY_PATH`
      (or equivalent) to the directory that contains the libvtk*.so
      libraries.  You must also set your `PYTHONPATH` to *both* the
      directory that contains all the `libvtk*Python.so` files *and*
      the `Wrapping/Python` directory.  Under bash/sh something like
      so needs to be done::

        $ export LD_LIBRARY_PATH=$LIBRARY_OUTPUT_PATH
        $ export PYTHONPATH=$VTK_ROOT/Wrapping/Python:${LIBRARY_OUTPUT_PATH}

      and under csh::

        $ setenv LD_LIBRARY_PATH ${LIBRARY_OUTPUT_PATH}
        $ setenv PYTHONPATH ${VTK_ROOT}/Wrapping/Python:${LIBRARY_OUTPUT_PATH}

      where VTK_ROOT is the directory where VTK is being built
      (`VTK_BINARY_DIR`) and `LIBRARY_OUTPUT_PATH` (this variable is
      set in `CMakeCache.txt`) is where the libraries are built.
      Change this to suit your configuration.

    Win32
      Similar to the Unix approach, set your `PATH` to point to the
      directory that contains the VTK DLL's and the `PYTHONPATH` to
      the directory that contains the Wrapping/Python directory and
      the DLL's.  Note that under Win32 the directory containing the
      DLL's is in a sub-directory of `LIBRARY_OUTPUT_PATH`.  The
      sub-directory is one of Release, Debug, MinSizeRel or
      RelWithDebInfo.


  (3) Installation via distutils to a directory different from the
      `VTK_ROOT` directory.  To install VTK built from source you
      simply need to run the "install rule".  Under Unix this
      implies running `make install` and under Windows this implies
      running the INSTALL target.  

      The installation rule internally executes the
      ``VTK_BINARY_DIR/Wrapping/Python/setup.py`` script.
      `setup.py` is a distutils script that should install
      VTK-Python correctly.  The `setup.py` script may also be
      executed from the `VTK_BINARY_DIR` in order to build an
      installer (via `bdist_wininst`) or to build a Python Egg.
  

VTK-Python interpreters
^^^^^^^^^^^^^^^^^^^^^^^

In order to solve some problems with running VTK-Python on some
platforms and compilers a special Python interpreter is distributed
along with VTK.  This new interpreter is called `vtkpython` and is
the recommended way of running VTK-Python scripts.  If the vanilla
Python interpreter is good enough and works well for you, please use
it.  However if you run into severe problems you might want to give
vtkpython a try.  Incidentally, to see what the problems are with
the vanilla Python interpreter on some platforms read this thread:
 
http://public.kitware.com/pipermail/vtk-developers/2002-May/001536.html

Additionally, if you built VTK along with MPI support
(`VTK_USE_PARALLEL` and `VTK_USE_MPI` are true) then another
VTK-Python interpreter called `pvtkpython` is also built.  This
interpreter initializes MPI correctly and is the recommended way to
run parallel VTK-Python scripts.


Structure/Usage
^^^^^^^^^^^^^^^

Once the Python packages are installed properly (either system wide
or by using environment variables) to use the new package structure
users will just do::

  import vtk
  # or
  from vtk import *

and all the available 'kits' will be loaded - just like with the
older vtkpython.  The name of the kits is available in the kits
variable::

  import vtk
  print vtk.kits
  ['common', 'filtering', 'io', ...]

If the user specifically wants just the classes the Common directory
imported the user does::

  import vtk.common

All the kit names are in lowercase.  This is similar to the way in
which the Tcl packages are split.  Similarly, classes specifically
in other kits can be imported by using the appropriate kit name.
Please do note that even when you import vtk.common, the vtk
namespace will still have all the kits loaded inside it.  Its just
that vtk.common will have only the classes from the Common
directory. 


Valid Kit names
~~~~~~~~~~~~~~~

Required Kits
-------------

common, filtering, io, imaging and graphics.

These are the required kits that you must have to use VTK.  You
can import all of them using the required module like so:

    from vtk.required import *

You should have all the required kits in your namespace.  If any
of them is not importable you *will* get an ImportError.

Optional Kits
-------------

genericfiltering, hybrid, parallel, rendering, volumerendering,
and widgets.

These are the optional kits.  Unlike the Tcl packages importing
these kits *will not* import all the required kits in as well.
For the rationale behind this please read this mail and also the
thread here:

http://public.kitware.com/pipermail/vtk-developers/2001-October/000828.html

If you don't have a particular optional kit then Python will not
raise an exception when importing vtk, but if you try loading it
directly like so::

    import vtk.parallel

Then you will receive an import error if there was one.  Also, if
the module exists but there are linking errors you will get a
LinkError exception.


Other VTK related modules
^^^^^^^^^^^^^^^^^^^^^^^^^

Apart from the basic VTK functionality there are other useful VTK
related modules in the package.  There are various widgets for
different GUI toolkits.  These are available in the various
sub-directories of the vtk directory.  At the time of this writing
the following widgets are available.

  gtk -- pyGTK widgets.

  qt -- PyQt v3 widgets.

  qt4 -- PyQt v4 widgets.

  tk -- The Tkinter widgets.  

  wx -- wxPython widgets.

The widgets can be used like so::

  from vtk.tk import vtkTkRenderWidget
  # or
  from vtk.gtk import GtkVTKRenderWindow
  # etc.

To see what widgets are available please look at the various
directories in the `vtk` sub-directory.

Apart from the GUI widgets there is a package called `vtk.util`.
This directory will contain miscellaneous modules that are useful
for different things.  Again, look at the directory to see what is
available.

There is also a `vtk.test` package that allows one to create unit
tests for VTK-Python.


Backwards compatibility
^^^^^^^^^^^^^^^^^^^^^^^

Since VTK-4.0, the usage of `vtkpython`, `vtkpythontk`,
`vtkTkRenderWidget` and other modules in the Wrapping/Python
directory were deprecated.  As of VTK-5.0, these files are no longer
available.  Please use the `vtk` package instead which provides the
functionality.


Writing and running VTK-Python tests
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The `vtk.test` package provides a module called `Testing` that
contains useful utility functions and a `vtkTest` class.  These ease
creation of VTK-Python unittests.  The file
`Wrapping/Python/vtk/test/Testing.py` is well documented and provides
more detailed information including pointers to examples.  VTK-Python
test writers are encouraged to use this module.

When you need to run a VTK-Python unittest that uses the
`vtk.test.Testing` module run the example with a `-h` (or `--help`)
argument to see all the supported options.  Typically you will need
to do something like the following::

  python TestTkRenderWidget.py -B $VTK_DATA_ROOT/Baseline/Rendering

To see the options available use this::

  python TestTkRenderWidget.py -h


Information for packagers
^^^^^^^^^^^^^^^^^^^^^^^^^

This section provides some details on how best to package VTK-Python
in the form of Debian, RPM and other packages.

  1. The binaries in the Build tree of VTK have RPATH information
     embeded. When an install is performed with 'make install' or
     when CPACK is used, all RPATH information is stripped from the
     binaries.

  2. VTK libraries (starting with VTK-5.0) are versioned.
     Therefore, it is safe to install multiple versions of VTK.
     Prior to VTK-5.0 this was not the case.

  3. VTK-Python has two components.  The ``libvtk*PythonD.so.*`` (or
     ``vtk*PythonD.dll``) files should be treated as libraries and
     *not* as Python extension modules.  Only the
     ``libvtk*Python.so`` (or ``vtk*Python.dll``) files are Python
     extension modules.  These extension modules are linked to the
     `PythonD.so` libraries.  

     Therefore, the ``libvtk*PythonD.so*`` files should be installed
     somewhere in the linkers path (for example in `\usr\lib`).
     Under Windows these should be installed in a directory that is
     in the `PATH`.  The Python extension modules should be
     installed via the `setup.py` file inside the `vtk` package.
     Typically these should be installed to
     `/usr/lib/pythonX.Y/site-packages/vtk` (or
     `PythonX.Y\Lib\site-packages\vtk`).

The VTK install rule (`make install` under Unix) will usually do the
right thing in installing everything.  Make sure that the
`CMAKE_INSTALL_PREFIX` variable is set appropriately.  There are two
ways to customize python module installation.  First, one may modify
the VTK_PYTHON_SETUP_ARGS CMake cache variable to set the options
passed to the setup.py script.  The default value for this variable
provides reasonable behavior for packagers.  Second, one may
define VTK_INSTALL_NO_PYTHON:BOOL=ON in the CMake cache which will
disable the automatic execution of setup.py as part of the install
process.  Then one may run ``python setup.py install`` manually
with the desired options.


Common problems
^^^^^^^^^^^^^^^

Some common problems are described in the VTK FAQ available here:

  http://public.kitware.com/cgi-bin/vtkfaq?req=home

The following lists a few serious problems and their solution from
the above FAQ.

Q. Why do I get the Python error -- ValueError: method requires a
VTK object?

A.  You just built VTK with Python support and everything went
smoothly. After you install everything and try running a Python-VTK
script you get a traceback with this error:

 ValueError: method requires a VTK object.

This error occurs if you have two copies of the VTK libraries on
your system. These copies need not be in your linkers path. This is
necessary to be able to test the build in place. When you install
VTK into another directory in your linkers path and then run a Python
script, the Python modules remember the old path and load the libraries
in the build directory as well. This triggers the above error since
the object you passed the method was instantiated from the other copy.

So how do you fix it? The easiest solution is to simply delete the
copy of the libraries inside your build directory or move the build
directory to another place. For example, if you build the libraries
in `VTK/bin` then move `VTK/bin` to `VTK/bin1` or remove all the
`VTK/bin/*.so` files. The error should no longer occur.

Alternatively, starting with recent VTK CVS versions (post Dec. 6,
2002) and with VTK versions greater than 4.1 (i.e. 4.2 and beyond)
there is a special VTK-Python interpreter built as part of VTK
called 'vtkpython' that should eliminate this problem.  Simply use
'vtkpython' in place of the usual 'python' interpreter when you use
VTK-Python scripts and the problem should not occur.  This is
because vtkpython uses the libraries inside the build directory.



Reporting Bugs
^^^^^^^^^^^^^^

If you have trouble or spot bugs please let us know at
vtk-developers@vtk.org

