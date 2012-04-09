set(Module_SRCS
  vtkBlueObeliskData.cxx
  vtkBlueObeliskDataParser.cxx
  vtkCMLMoleculeReader.cxx
  vtkMoleculeAlgorithm.cxx
  vtkMoleculeMapper.cxx
  vtkMoleculeToAtomBallFilter.cxx
  vtkMoleculeToBondStickFilter.cxx
  vtkMoleculeToPolyDataFilter.cxx
  vtkPeriodicTable.cxx
  vtkProgrammableElectronicData.cxx
  vtkSimpleBondPerceiver.cxx
  )

set_source_files_properties(
  vtkMoleculeToPolyDataFilter.cxx
  ABSTRACT
  )

set_source_files_properties(
  vtkBlueObeliskData.cxx
  vtkBlueObeliskDataParser.cxx
  WRAP_EXCLUDE
  )

# Generate configured header file
configure_file(${vtkChemistryCore_SOURCE_DIR}/vtkChemistryConfigure.h.in
  ${vtkChemistryCore_BINARY_DIR}/vtkChemistryConfigure.h IMMEDIATE)

set(${vtk-module}_NO_HeaderTest 1) # TODO: Fix headers and enable test.
vtk_module_library(vtkChemistryCore ${Module_SRCS})
