# Added more VTK-m accelerated filter overrides

Factory overrides for the following filters have been added:
1. `vtkTableBasedClipDataSet` can be overriden by `vtkmClip`
2. `vtkCutter` can be overriden by `vtkmSlice`
3. `vtkThreshold` can be overriden by `vtkmThreshold`

These overrides are available when the `VTK::AcceleratorsVTKmFilters` is enabled
and the `VTK_ENABLE_VTKM_OVERRIDES` cmake option may be turned on.

Note that for these overrides to be used, the `VTK::AcceleratorsVTKmFilters`
module must be linked (for C++) or imported (for Python).
