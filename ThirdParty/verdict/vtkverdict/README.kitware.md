# verdict fork for VTK

This branch contains changes required to embed verdict in VTK.

* Ignore whitespace for VTK's commit checks.
* Add VTK-specific CMakeLists.txt to integrate into VTK's build system.
* mangle namespace `verdict` as `vtkverdict`
