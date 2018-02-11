set(Module_SRCS
  vtkConvertSelection.cxx
  vtkExtractBlock.cxx
  vtkExtractCells.cxx
  vtkExtractDataArraysOverTime.cxx
  vtkExtractDataOverTime.cxx
  vtkExtractDataSets.cxx
  vtkExtractEdges.cxx
  vtkExtractGeometry.cxx
  vtkExtractGrid.cxx
  vtkExtractLevel.cxx
  vtkExtractPolyDataGeometry.cxx
  vtkExtractRectilinearGrid.cxx
  vtkExtractSelectedArraysOverTime.cxx
  vtkExtractSelectedBlock.cxx
  vtkExtractSelectedIds.cxx
  vtkExtractSelectedLocations.cxx
  vtkExtractSelectedPolyDataIds.cxx
  vtkExtractSelectedRows.cxx
  vtkExtractSelectedThresholds.cxx
  vtkExtractSelection.cxx
  vtkExtractTemporalFieldData.cxx
  vtkExtractTensorComponents.cxx
  vtkExtractTimeSteps.cxx
  vtkExtractUnstructuredGrid.cxx
  vtkExtractVectorComponents.cxx
  vtkHierarchicalDataExtractDataSets.cxx
  vtkHierarchicalDataExtractLevel.cxx
  vtkProbeSelectedLocations.cxx
  )

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND Module_SRCS
    vtkExtractArraysOverTime.cxx)
endif()

vtk_module_library(vtkFiltersExtraction ${Module_SRCS})
