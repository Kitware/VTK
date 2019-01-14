set(classes
  vtkBlueObeliskData
  vtkBlueObeliskDataParser
  vtkCMLMoleculeReader
  vtkGaussianCubeReader2
  vtkMoleculeMapper
  vtkMoleculeToAtomBallFilter
  vtkMoleculeToBondStickFilter
  vtkMoleculeToLinesFilter
  vtkMoleculeToPolyDataFilter
  vtkPeriodicTable
  vtkPointSetToMoleculeFilter
  vtkProgrammableElectronicData
  vtkProteinRibbonFilter
  vtkSimpleBondPerceiver
  vtkVASPAnimationReader
  vtkVASPTessellationReader
  vtkXYZMolReader2)

# Generate configured header file
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkChemistryConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkChemistryConfigure.h"
  @ONLY)

vtk_module_add_module(VTK::DomainsChemistry
  CLASSES ${classes}
  HEADERS "${CMAKE_CURRENT_BINARY_DIR}/vtkChemistryConfigure.h"
  PRIVATE_HEADERS vtkBlueObeliskDataInternal.h)
