This directory contains the new interface to the VTK Python modules.
This code hopefully doesn't break any existing code so you can
continue using vtkpython.py and others.  Have no fear, we plan to keep
things backwards compatible in the future.

Warning:
^^^^^^^^

  This code is all experimental and still needs testing.  So please
  don't use this for production code.  Things might change and files
  removed/renamed/re-organized.  However, this code needs testing so
  please do check it out and give us feedback at
  vtk-developers@public.kitware.com

Structure/Usage:
^^^^^^^^^^^^^^^^

  First, you will need to have the vtk_src_root/Wrapping/Python in
  your PYTHONPATH.  Normal users will just do

    import vtk
    # or
    from vtk import *

  and all the available 'kits' will be loaded - just like with
  vtkpython.  The name of the kits is available in the kits variable.

    import vtk
    print vtk.kits

  If the user specifically wants just the classes from say the Common
  directory imported the user does:

    import vtk.common

  All the kit names are in lowercase.  This is similar to the way in
  which the Tcl packages are split.  Similarly classes specifically in
  other kits can be loaded by using the appropriate kit name.  The
  problem with this is that when you import vtk.common, the vtk
  namespace will still have all the kits loaded inside it.  Its just
  that vtk.common will have only the classes from the Common
  directory.  If folks object to this behaviour things might change in
  the future.


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
    directlly like so:

        import vtk.parallel

    Then you will receive an import error if there was one.  Also, if
    the module exists but there are linking errors you will get a
    LinkError exception.


Testing:
^^^^^^^^

  The testing directory isnt populated yet but will soon have code
  that will be useful for testing the VTK Python modules.


Widgets/Interaction:
^^^^^^^^^^^^^^^^^^^^

  The name for the widget/interaction package directory hasn't been
  decided yet but this directory will have the special TkWidgets
  (RenderWidget, ImageWidget etc.), GTK Widgets and other relavant
  modules.  This is still subject to discussion.

