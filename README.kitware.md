# {fmt} fork for VTK

This branch contains changes required to embed {fmt} in VTK.

* Ignore whitespace for VTK's commit checks.
* Add VTK-specific CMakeLists.txt to integrate into VTK's build system.
* mangle namespace `fmt` as `vtkfmt`

