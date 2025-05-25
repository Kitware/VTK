set(classes
  vtkRemoteSession
  vtkStandaloneSession)

vtk_module_add_module(VTK::WebAssemblySession
  PRIVATE_HEADERS "vtkWebAssemblySessionHelper.h"
  CLASSES ${classes})

vtk_add_test_mangling(VTK::WebAssemblySession)
