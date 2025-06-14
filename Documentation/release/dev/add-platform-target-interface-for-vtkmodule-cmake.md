## Add PLATFORM_TARGET argument for vtkModule CMake APIs

The `vtk_module_build` function now supports a `PLATFORM_TARGET` keyword argument. This allows you to specify a target that provides platform-specific compile and link flags. These flags will be automatically propagated to consumers of VTK modules, ensuring consistent builds across different platforms.
