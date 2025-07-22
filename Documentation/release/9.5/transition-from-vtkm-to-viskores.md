# Transition from VTKm to Viskores

VTK has transitioned its dependency from [VTKm][vtkm] to [Viskores][viskores].
This transition involves several important changes:

- VTK now depends on Viskores instead of VTKm
- Fides has been updated to also transition its dependency to Viskores
- The accelerator vtkm VTK module has been modified to use Viskores in its implementation
- VTK Accelerator VTKm API remains fully compatible for downstream users
- No code changes should be required for applications using the VTK Accelerator API

## Build Breaking Change

- The flag `VTK_ENABLE_VISKORES_OVERRIDES` has been introduced * This new flag
  deprecates the previous `VTK_ENABLE_VTKM_OVERRIDES`.
- Projects using the previous flag in their build system will need to update accordingly

[vtkm]: https://gitlab.kitware.com/vtk/vtk-m
[viskores]: https://github.com/viskores/viskores
