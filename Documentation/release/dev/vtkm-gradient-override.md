## Added vtkGradientFilter -> vtkmGradient override

Added `vtkGradientFilter` to the list of filters that are overriden by a VTK-m based
implementation (`vtkmGradient`) when the `VTK::AcceleratorsVTKmFilters` module is
enabled and the CMake option `VTK_ENABLE_VTKM_OVERRIDES` is set.
