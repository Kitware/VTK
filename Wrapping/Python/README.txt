                   VTK-Python Package Documentation
                   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This documents the new Python package structure for the VTK-Python
modules.

Every effort has been taken to keep the changes backwards compatible
so your old code should work without any problems.  However, we
recommend that you use the new package structure.


Contents:
^^^^^^^^^

  Installation

  Structure/Usage
 
  Other VTK related modules

  Backwards compatibility

  Information for packagers

  Reporting Bugs


Installation:
^^^^^^^^^^^^^

  There are primarily two ways of using the VTK-Python wrappers and
  modules.  

    (1) Using the package from the source build without installing it
    system wide.  This is most useful when you build VTK off a CVS
    checkout and do not want to install it system wide.  This is also
    useful if you are not the administrator of the machine you are
    using/building VTK on.

     *nix:

      Under Unix the way to do this is to export LD_LIBRARY_PATH (or
      equivalent) to the directory that contains the libvtk*.so
      libraries.  You must also export your PYTHONPATH to _both_ the
      directory that contains all the libvtk*Python.so files _and_ the
      Wrapping/Python directory.  Under bash/sh something like so
      needs to be done:
    
      $ export LD_LIBRARY_PATH=$VTK_ROOT/lib
      $ export PYTHONPATH=$VTK_ROOT/Wrapping/Python:$VTK_ROOT/lib

      where VTK_ROOT is the directory where your VTK sources are the
      VTK_ROOT/lib directory is where the libraries are built.  Change
      this to suit your configuration.

     Win32:

      Similar to the Unix approach, set your PATH to point to the
      directory that contains the VTK DLL's and the PYTHONPATH to the
      directory that contains the Wrapping/Python directory and the
      DLL's.


    (2) Installation via distutils to a directory different from the
    build directory.  The file VTK_ROOT/Wrapping/Python/setup.py is a
    distutils script that should install VTK-Python correctly.  It
    should work under *nix, Win32 and possibly other platforms.

    Under *nix please note that you must first run 'make install' to
    install the libraries to system wide directories.  To install the
    VTK-Python wrappers just do the following:

       # cd VTK_ROOT/Wrapping/Python
       # python setup.py install
    
    The usual options for distutils installers are all available.  So
    it is possible to make a non system wide install by using the
    --prefix option.

    If you have trouble with this script or find bugs please mail us
    at <vtk-developers@public.kitware.com> or at
    <vtkusers@public.kitware.com>.


Structure/Usage:
^^^^^^^^^^^^^^^^

  Once the Python packages are installed properly (either system wide
  or by using environment variables) to use the new package structure
  users will just do

    import vtk
    # or
    from vtk import *

  and all the available 'kits' will be loaded - just like with the
  older vtkpython.  The name of the kits is available in the kits
  variable.

    import vtk
    print vtk.kits
    ['common', 'filtering', 'io', ...]

  If the user specifically wants just the classes the Common directory
  imported the user does:

    import vtk.common

  All the kit names are in lowercase.  This is similar to the way in
  which the Tcl packages are split.  Similarly, classes specifically
  in other kits can be imported by using the appropriate kit name.
  Please do note that even when you import vtk.common, the vtk
  namespace will still have all the kits loaded inside it.  Its just
  that vtk.common will have only the classes from the Common
  directory. 


 Valid Kit names:
 ^^^^^^^^^^^^^^^^

  Required Kits:
  --------------

    common, filtering, io, imaging and graphics.

    These are the required kits that you must have to use VTK.  You
    can import all of them using the required module like so:

	from vtk.required import *

    You should have all the required kits in your namespace.  If any
    of them is not importable you *will* get an ImportError.

  Optional Kits:
  -------------

    rendering, hybrid, patented and parallel:

    These are the optional kits.  Unlike the Tcl packages importing
    these kits *will not* import all the reqiuired kits in as well.
    For the rationale behind this please read this mail and also the
    thread here:

http://public.kitware.com/pipermail/vtk-developers/2001-October/000828.html

    If you don't have a particular optional kit then Python will not
    raise an exception when importing vtk, but if you try loading it
    directly like so:

        import vtk.parallel

    Then you will receive an import error if there was one.  Also, if
    the module exists but there are linking errors you will get a
    LinkError exception.


Other VTK related modules:
^^^^^^^^^^^^^^^^^^^^^^^^^^

  Apart from the basic VTK functionality there are other useful VTK
  related modules in the package.  There are various widgets for
  different GUI toolkits.  These are available in the various
  sub-directories of the vtk directory.  At the time of this writing
  the following widgets are available.

    gtk -- pyGTK widgets.

    qt -- Qt widgets.

    tk -- The Tkinter widgets.  

    wx -- wxPython widgets.

  The widgets can be used like so:

    from vtk.tk import vtkTkRenderWidget
    # or
    from vtk.gtk import GtkVTKRenderWindow
    # etc.

  To see what widgets are available please look at the various
  directories in the vtk sub-directory.

  Apart from the GUI widgets there is a directory called util.  This
  directory will contain miscellaneous modules that are useful for
  different things.  Again, look at the directory to see what is
  available.

  There is also a test module that allows one to create unit tests for
  VTK-Python.


Backwards compatibility:
^^^^^^^^^^^^^^^^^^^^^^^^

  All the older modules like vtkpython etc. are still available.  This
  means that all your old code should still run ok.  However, new
  additions like the pyGTK widget will only be available via the new
  package structure.  To illustrate the point consider the
  wxVTKRenderWindow.py module.  To import this you can do either

    from vtk.wx import wxVTKRenderWindow
    # or
    import wxVTKRenderWindow

  The wxVTKRenderWindow and other older modules have been suitably
  modified to use the new structure.


Information for packagers:
^^^^^^^^^^^^^^^^^^^^^^^^^^


  Sometimes while building RPMS or debs the provided distutils
  setup.py script might not do.  Here is some information for such
  people who are unable to use the setup.py script.

    (1) The entire contents of this directory and its sub-directories
    could be placed in a system wide directory called 'vtkpython' (or
    something else).  Additionally you will need to create a vtk.pth
    that points to this directory.  Lets say you install the modules
    in /usr/lib/python2.1/site-packages/vtkpython.  Then create a
    vtk.pth that contains just one line containing just the word
    'vtkpython' (without the quotes) in it and place that in
    /usr/lib/python2.1/site-packages/.

    (2) You must ensure that the VTK Python libraries are in both your
    PYTHONPATH and in your linkers path.  Under Unix this means that
    the files libvtkCommonPython.so (libvtk*Python.so) are both in
    your PYTHONPATH and in the LD_LIBRARY_PATH (or in ld.so.conf).

    (3) Under GNU/Linux both (1) and (2) are achievable if you do
    something like so:

     Method 1:

      (a) Put all the Python modules in /usr/lib/vtk/python.

      (b) Put libvtk*Python.so also in /usr/lib/vtk/python.

      (c) Add the /usr/lib/vtk/python directory to /etc/ld.so.conf.

      (d) Create a vtkpython.pth in /usr/lib/pythonX.Y/site-packages/
      with the single entry '/usr/lib/vtk/python' (without the
      quotes).

    Alternatively, the following approach might be preffered (this is
    infact exactly what the setup.py script does):

     Method 2:

      (a) Put all the Python modules in 
        /usr/lib/pythonX.Y/site-packages/vtk_python/.

      (b) Put libvtk*Python.so also in /usr/lib/.

      (c) Create a vtkpython.pth in /usr/lib/pythonX.Y/site-packages/
      with the single entry 'vtk_python' (without the quotes).

      (d) Make symbolic links from
      /usr/lib/pythonX.Y/site-packages/vtk_python/ to
      /usr/lib/libvtk*Python.so  i.e.

      # ln -s /usr/lib/libvtk*Python.so /usr/lib/pythonX.Y/site-packages/vtk_python/
   

  This should give you a working VTK-Python setup.


Reporting Bugs:
^^^^^^^^^^^^^^~

If you have trouble or spot bugs please let us know at
vtk-developers@public.kitware.com

