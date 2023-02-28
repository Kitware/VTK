vtk_module_find_package(
  PACKAGE OpenCASCADE
  COMPONENTS
    FoundationClasses
    ModelingData
    ModelingAlgorithms
    Visualization
    ApplicationFramework
    DataExchange
)

vtk_module_add_module(VTK::IOOCCT
  CLASSES
    vtkOCCTReader
)

vtk_module_link(VTK::IOOCCT PRIVATE TKXDESTEP TKXDEIGES)

# OpenCASCADE started putting include directory usage requirements in 7.7.0.
set(OpenCASCADE_VERSION
  "${OpenCASCADE_MAJOR_VERSION}.${OpenCASCADE_MINOR_VERSION}.${OpenCASCADE_MAINTENANCE_VERSION}")
if (OpenCASCADE_VERSION VERSION_LESS "7.7.0")
  vtk_module_include(VTK::IOOCCT PRIVATE "${OpenCASCADE_INCLUDE_DIR}")
endif ()

vtk_add_test_mangling(VTK::IOOCCT)
