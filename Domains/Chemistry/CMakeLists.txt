set(Module_SRCS
  vtkBlueObeliskData.cxx
  vtkBlueObeliskDataInternal.h
  vtkBlueObeliskDataParser.cxx
  vtkCMLMoleculeReader.cxx
  vtkGaussianCubeReader2.cxx
  vtkMoleculeAlgorithm.cxx
  vtkMoleculeMapper.cxx
  vtkMoleculeToAtomBallFilter.cxx
  vtkMoleculeToBondStickFilter.cxx
  vtkMoleculeToPolyDataFilter.cxx
  vtkPeriodicTable.cxx
  vtkProgrammableElectronicData.cxx
  vtkProteinRibbonFilter.cxx
  vtkSimpleBondPerceiver.cxx
  vtkVASPAnimationReader.cxx
  vtkVASPTessellationReader.cxx
  vtkXYZMolReader2.cxx
  )

set_source_files_properties(
  vtkMoleculeToPolyDataFilter.cxx
  ABSTRACT
  )

set_source_files_properties(
  vtkBlueObeliskData.cxx
  vtkBlueObeliskDataInternal.h
  vtkBlueObeliskDataParser.cxx
  PROPERTIES
    WRAP_EXCLUDE 1
  )

set_source_files_properties(
  vtkBlueObeliskDataInternal.h
  PROPERTIES
    WRAP_EXCLUDE_PYTHON 1
  )

# Generate configured header file
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/vtkChemistryConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkChemistryConfigure.h")

set(${vtk-module}_NO_HeaderTest 1) # TODO: Fix headers and enable test.
vtk_module_library(${vtk-module} ${Module_SRCS})
