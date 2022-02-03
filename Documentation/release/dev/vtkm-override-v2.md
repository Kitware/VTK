## Some VTK-m filters now override VTK filters using object factories

When building with `VTK::AcceleratorsVTKmFilters` module enabled, a new CMake option called
`VTK_ENABLE_VTKM_OVERRIDES` is available which can be turned on to enable overriding
certain VTK filters with their VTK-m counterparts. Note that when building your own code,
it must be linked with `VTK::AcceleratorsVTKmFilters`, and the cmake function
`vtk_module_autoinit` should be used, for the factory overrides to be available.

The override also needs to be toggled at runtime using the static function
`vtkmFilterOverrides::SetEnabled(bool)`.

Currently, both the build and run-time flags are `off` by default. The CMake option is
marked as advanced.

Currently the following overrides are available:
1. vtkContourFilter --> vtkmContour
