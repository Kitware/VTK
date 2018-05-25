set(Module_SRCS
  vtkBlueObeliskData.cxx
  vtkBlueObeliskDataInternal.h
  vtkBlueObeliskDataParser.cxx
  vtkCMLMoleculeReader.cxx
  vtkGaussianCubeReader2.cxx
  vtkMoleculeMapper.cxx
  vtkMoleculeToAtomBallFilter.cxx
  vtkMoleculeToBondStickFilter.cxx
  vtkMoleculeToLinesFilter.cxx
  vtkMoleculeToPolyDataFilter.cxx
  vtkPeriodicTable.cxx
  vtkPointSetToMoleculeFilter.cxx
  vtkProgrammableElectronicData.cxx
  vtkProteinRibbonFilter.cxx
  vtkSimpleBondPerceiver.cxx
  vtkVASPAnimationReader.cxx
  vtkVASPTessellationReader.cxx
  vtkXYZMolReader2.cxx
  )

# Generate configured header file
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/vtkChemistryConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkChemistryConfigure.h")

set(${vtk-module}_NO_HeaderTest 1) # TODO: Fix headers and enable test.
vtk_module_library(${vtk-module} ${Module_SRCS})
