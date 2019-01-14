set(classes
  vtkBlockSelector
  vtkConvertSelection
  vtkExtractBlock
  vtkExtractCells
  vtkExtractCellsByType
  vtkExtractDataArraysOverTime
  vtkExtractDataOverTime
  vtkExtractDataSets
  vtkExtractEdges
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
  vtkExtractTemporalFieldData
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
    vtkExtractArraysOverTime
    vtkExtractSelectionLegacy)
endif ()

vtk_module_add_module(VTK::FiltersExtraction
  CLASSES ${classes})
