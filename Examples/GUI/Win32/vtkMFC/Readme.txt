vtkMFC : Example MDI and SDI programs.
Contributed by Andrew J, P, Maclean

The MDI_Instructions.chm and SDI_Instructions.chm help files included in 
this directory refer to building/creating MSVC project files manually.

It is preferable to use CMake to generate project files for these demos.

To do this, run cmake and use the project directory of 
your_path_here/VTK/Examples/GUI/Win32/vtkMFC
after CMake has completed, check the build directory that you chose in CMake 
and there will be project files for either MSVC 6 or MSVC 7 etc depending 
upon your output/compiler selection.

You can now load these projects into VC and build them.