## Move mesh cache utilities to new module

The vtkDataObjectMeshCache and vtkMeshCacheRunner should be available
from most modules, specially from Filters modules but not only.
So we want to move it in a Common module.
CommonMisc may be a candidate, but it leads to some module scan issue (see below)
Also, we prefer not flooding CommonCore with too much new features.
So we create a new module, CommonCache.

Typical error message:
> CMake Error at CMake/vtkModule.cmake:2911 (message):
  The VTK::CommonDataModel dependency is missing for VTK::CommonMisc.  The
  `vtk_module_scan` for this module used `ENABLE_TESTS WANT`.  This is a
  known issue, but the fix is not trivial.  You may either change the flag
  used to control testing for this scan or explicitly enable the
  VTK::CommonDataModel module.
Call Stack (most recent call first):
  CMakeLists.txt:631 (vtk_module_build)
