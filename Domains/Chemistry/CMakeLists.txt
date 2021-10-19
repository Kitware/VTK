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

set(headers
  vtkChemistryConfigure.h)

vtk_module_add_module(VTK::DomainsChemistry
  CLASSES ${classes}
  NOWRAP_HEADERS ${headers}
  PRIVATE_HEADERS vtkBlueObeliskDataInternal.h)
