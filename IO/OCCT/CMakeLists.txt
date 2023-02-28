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

vtk_add_test_mangling(VTK::IOOCCT)
