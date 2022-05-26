## VTK-m filter override runtime flag enabled by default

When building with `VTK::AcceleratorsVTKmFilters` module enabled, a CMake option called
`VTK_ENABLE_VTKM_OVERRIDES` can be turned on to enable overriding existing VTK filters,
like `vtkContourFilter`, using VTK's factory mechanism.

There is also a run-time switch that can be used to enable/disable the factory override
at runtime. It is available through the function `vtkmFilterOverrides::SetEnabled(bool)`.
The runtime switch is now `On` by default.
