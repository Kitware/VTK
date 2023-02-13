# Updated VTK-m submodule to VTK-m 2.0.0

We have updated the VTK-m git submodule distributed in the VTK source code to its
latest release, VTK-m 2.0.0. Being a major update, it significantly breaks
compatibility with the API provided by VTK-m 1.X. Thus, many changes were needed
in VTK to make it compatible with VTK-m 2.0.0.

Changes are introduced in three different components:

The VTK-m git submodule included in the VTK source which:

- Has been updated to VTK-m 2.0.0.

- It now exposes in the VTK project a different VTK-m cmake target naming scheme
  in which now every cmake target is prefixed by `vtkm_`.
  An exception has been made for `vtkm::cuda` and `vtkm::kokkos_hip`. This is
  because we also need to support VTK builds with external VTK-m, and when VTK-m
  2.0.0 is imported using find_project it will use the `vtkm::` prefix as opposed
  to the `vtkm_` prefix.

- The VTK-m VTK module is now called vtk::vtkvtkm as opposed to vtk::vtkm.

The VTK Accelerator module has also been updated to reflect the changes in VTK-m
2.0.0 in the following ways:

-  Now the vtkmlib module functions that translate VTK to VTK-m data
   structures takes into consideration changes such that the coordinates
   systems are now represented as one more field inside the VTK-m dataset
   rather than an special and unique component. This required many changes.
   Changes in the naming of the VTK-m headers files.

-  Refactor in the VTK-m Accelerator module for VTK which gets rid of not longer
   needed workarounds.

The Fides library has been updated upstream to ensure compatibility with VTK-m
2.0.0. This update in upstream has been brought to VTK to enable using VTK 2.0.0
and Fides through VTK.
