Some Tips for Borland (c++ builder) and vtk (4.x)
-----------------------------

Package Directory
-----------------
This contains a simple package which includes a Renderwindow Component
Build the package and install it to the palette. If you have trouble loading
the package project. Just create a new one and add in the code.
If you can't get the package built or you do not knowhow to do this, then 
you ought to learn about C++Builder before you play with vtk.
(The package project is CBuilder 6, if you are using CBuilder 5, just create 
an empty package and add vtkBorlandRenderwindow into it, then compile and 
install)
Package creation steps
1) Create new package
2) Add Renderwindow cpp file
3) Add include directories to project, vtk\Common, vtk\Filtering
vtk\Graphics, vtl\Imaging, vtk\Rendering, vtk\IO, vtk\Hybrid
also add the place you built vtk in for the vtkConfigure.h file.
4) Add conditional define STRICT
5) add vtkCommon.lib vtkFiltering.lib vtkGraphics.lib vtkIO.lib vtkRendering.lib
etc to your project.

ProjectDemo Directory
---------------------
A simple project that uses the Renderwindow Component above. The project files
have been created in C++Builder 6 and you may need to recreate them if you are 
using an earlier version of C++Builder. In particular if you get link errors to
rtl.bpi or rtl.lib, then these used to be named rtl50.bpi and vcl50.bpi etc
The best thing to do is delete the project, create a new one, add in the files 
and off you go.
The demo project is very simple and should get you started.
Steps for building the demo project are the same as the package above, however
you will need to "use" the package too.

Conditional Defines
-------------------
You should use conditional defines STRICT; in each vtk based project you 
work on. It may help solve some linking problems. STRICT causes the compiler 
to strongly type HWND and prevents a compiler cough caused by defining
certain functions twice (because it thinks void* and HWND are the same 
otherwise)


Directories
-----------
You must ensure that your project/options/directories includes the correct
vtk directory paths. eg
D:\vtk\Common;D:\vtk\Filtering;D:\vtk\Imaging;D:\vtk\Graphics;D:\vtk\IO;
D:\vtk\Rendering;D:\vtk\Hybrid;
and you also ned to add in something like
D:\vtkbuild or wherever you build vtk into using cmake because the file
vtkConfigure.h is place there and you need to include it.

make sure that you're projects link to the vtk libs and have the correct
directory in your lib dirs, you will usually need to link to
vtkCommon.lib; vtkFiltering.lib; vtkImaging.lib; vtkGraphics.lib; vtkO.lib;
vtkRendering.lib; vtkHybrid.lib

You can save some trouble by getting a project working with a minimal setup
and adding it to your repository as a vtk_project_template, this may save
redoing directories and stuff frequently.


