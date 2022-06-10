set(classes
  vtkConduitArrayUtilities
  vtkConduitSource
  vtkDataObjectToConduit)

# SDK component is required for Catalyst implementation (see Catalyst subdir)
vtk_module_find_package(PACKAGE catalyst VERSION 2.0 COMPONENTS SDK)

set_property(GLOBAL PROPERTY vtk_catalyst_dir_primary "${catalyst_DIR}")

vtk_module_add_module(VTK::IOCatalystConduit
  CLASSES ${classes})

vtk_module_link(VTK::IOCatalystConduit
  PUBLIC
    catalyst::catalyst)

vtk_add_test_mangling(VTK::IOCatalystConduit)
add_subdirectory(Catalyst)
