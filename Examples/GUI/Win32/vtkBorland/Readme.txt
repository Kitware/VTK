Some Tips for Borland (C++ Builder) and VTK (4.n)
-------------------------------------------------

Package Directory
-----------------
This contains a simple package which includes a RenderWindow Component.
Build the package and install it to the palette. If you have trouble loading
the package project, just create a new one and add in the code.
If you can't get the package built or you do not know how to do this, then
you ought to learn about C++ Builder before you play with VTK (the package
project is CBuilder 6, if you are using CBuilder 5, just create an empty
package and add vtkBorlandRenderWindow into it, then compile and install).

Package creation steps
----------------------
The following steps apply specifically to the Borland C++ Builder
(BCB) 5 IDE, but may work with later/earlier versions of the IDE.
These steps will establish a BCB package with one component, a
vtkBorlandRenderWindow, which may be dropped onto a form (etc.)
during GUI development in the IDE.  These steps bypass the use of
and enable one to overwrite/create the package from scratch using the
unit file /vtkBorland/Package/vtkBorlandRenderWindow.cpp and its
header file /vtkBorland/Package/vtkBorlandRenderWindow.h.

In the Borland C++ Builder IDE:

1. Component->Install Component
   Into new package
   Unit file name:   (Browse)
     /vtkBorland/Package/vtkBorlandRenderWindow.cpp
   Package file name: (Browse for directory and type)
     vtkBorlandRenderWindowPkg
   Package description: (as a suggestion)
     VTK-Borland render window
     a Confirm dialogue pops up:
     'Package vtkBorlandRenderWindowPkg will be built then installed'
     choose no
2. Options->Compiler
     choose debug or release
3. Options->Directories/Conditionals
   Include path:
     /VTK/Common
     /VTK/Filtering
     /VTK/Rendering
     path to vtkConfigure.h
   Library path:
     path to vtk libs
   Conditionals:
   Add
     'STRICT'
4. Add->Add Unit->Browse
   Files of type:
     choose Library file (*.lib)
     find and choose
       vtkCommon.lib
       vtkFiltering.lib
       vtkRendering.lib
5. Compile
6. Install
     an Information popup should appear:
     'Package ...vtkBorlandRenderWindowPkg.bpl has been installed.
     The following new component(s) have been registered: TvtkBorlandRenderWindow.'
7. File->Close All
     choose Save
8. Check that there is an icon for the component on the palette under the Samples tab


ProjectDemo Directory
---------------------
A simple project is provided that uses the vtkBorlandRenderWindow Component
above.  The project files have been created in C++ Builder 6 and you may need to
delete/re-create them if you are using an earlier version of C++ Builder.
In particular, if you get link errors to rtl.bpi or rtl.lib, these used
to be named rtl50.bpi and vcl50.bpi, etc.

The best thing to do is delete the project, create a new one, add in the files
and off you go.  The demo project is very simple and should get you started.
Steps for building the demo project are the same as the package above, however
you will need to "use" the package too.

Building the example in the IDE:

1. File->New Application
     this creates a blank form and unit
2. Project->Add to Project
     browse and add the unit ..\vtkBorland\ProjectDemo\Form_Test.cpp
3. Project->Add to Project
     browse and add the unit ..\vtkBorland\ProjectDemo\Form_Test.cpp
4. Project->Remove from Project
     choose the default created 'Unit1.cpp'
5. Project->Options->Compiler
     choose debug or release
6. Project->Options->Directories/Conditionals
   Include path:
     path to/VTK/Common
     path to/VTK/Filtering
     path to/VTK/Graphics
     path to/VTK/Rendering
     path to vtkConfigure.h
   Library path:
     path to vtk libs
   Conditionals:
   Add
     'STRICT'
7. File->Save Project As
     choose a name and save (e.g., Project1)
8. Project->Build Project1
     debug as required
9. Project->Run



Conditional Defines
-------------------
You should use conditional defines STRICT; in each vtk based project you
work on. It may help solve some linking problems. STRICT causes the compiler
to strongly type HWND and prevents a compiler cough caused by defining
certain functions twice (because it thinks void* and HWND are the same
otherwise).


Directories
-----------
You must ensure that your project/options/directories includes the correct
VTK directory paths: e.g.,
D:\vtk\Common;D:\vtk\Filtering;D:\vtk\Imaging;D:\vtk\Graphics;D:\vtk\IO;
D:\vtk\Rendering;D:\vtk\Hybrid;
and you also need to add in something like
D:\vtkbuild or wherever you build VTK into using cmake because the file
vtkConfigure.h is placed there and you need to include it.

Make sure that you're projects link to the VTK libs and have the correct
directory in your lib dirs.  You will usually need to link to
vtkCommon.lib; vtkFiltering.lib; vtkImaging.lib; vtkGraphics.lib; vtkO.lib;
vtkRendering.lib; vtkHybrid.lib.

You can save some trouble by getting a project working with a minimal setup
and adding it to your repository as a vtk_project_template.  This may save
re-doing directories and stuff frequently.


