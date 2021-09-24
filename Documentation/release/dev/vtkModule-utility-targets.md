## vtkModule-utility-targets

The `vtk_module_build`, `vtk_module_wrap_python`, and `vtk_module_wrap_java`
APIs now support a `UTILITY_TARGET` argument. All libraries and executables
created under its control will link privately to this target. This may be used
to ensure that compiler or link flags are consistently passed to code built
under the module system.

The target must be exported manually (but can be added to the same export set
as `vtk_module_build` uses).
