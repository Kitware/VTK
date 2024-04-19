set(classes
  vtkBlueObeliskData
  vtkBlueObeliskDataParser
  vtkMoleculeMapper
  vtkMoleculeToAtomBallFilter
  vtkMoleculeToBondStickFilter
  vtkMoleculeToLinesFilter
  vtkMoleculeToPolyDataFilter
  vtkPeriodicTable
  vtkPointSetToMoleculeFilter
  vtkProgrammableElectronicData
  vtkProteinRibbonFilter
  vtkSimpleBondPerceiver)

vtk_module_add_module(VTK::DomainsChemistry
  CLASSES ${classes}
  PRIVATE_HEADERS vtkBlueObeliskDataInternal.h)
vtk_add_test_mangling(VTK::DomainsChemistry)
