set(classes
  vtkBlockSelector
  vtkConvertSelection
  vtkExpandMarkedElements
  vtkExtractBlock
  vtkExtractCells
  vtkExtractCellsByType
  vtkExtractDataArraysOverTime
  vtkExtractDataOverTime
  vtkExtractDataSets
  vtkExtractEdges
  vtkExtractExodusGlobalTemporalVariables
  vtkExtractGeometry
  vtkExtractGrid
  vtkExtractLevel
  vtkExtractPolyDataGeometry
  vtkExtractRectilinearGrid
  vtkExtractSelectedArraysOverTime
  vtkExtractSelectedBlock
  vtkExtractSelectedIds
  vtkExtractSelectedLocations
  vtkExtractSelectedPolyDataIds
  vtkExtractSelectedRows
  vtkExtractSelectedThresholds
  vtkExtractSelection
  vtkExtractTensorComponents
  vtkExtractTimeSteps
  vtkExtractUnstructuredGrid
  vtkExtractVectorComponents
  vtkFrustumSelector
  vtkHierarchicalDataExtractDataSets
  vtkHierarchicalDataExtractLevel
  vtkLocationSelector
  vtkProbeSelectedLocations
  vtkSelector
  vtkValueSelector)

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND classes
    vtkExtractTemporalFieldData)
endif()

vtk_module_add_module(VTK::FiltersExtraction
  CLASSES ${classes})
