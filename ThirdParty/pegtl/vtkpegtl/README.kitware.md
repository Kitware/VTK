# tao/PEGTL fork for VTK

This branch contains changes required to embed PEGTL into VTK.
This includes changes to the build system and the following:

* add .gitattribues
* Integrate VTK's module system (added CMakeLists.vtk.txt)

No mangling is needed since this is header only and not intended
for use in public API in VTK.
