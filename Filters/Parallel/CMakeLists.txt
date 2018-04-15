set(Module_SRCS
  vtkAggregateDataSetFilter.cxx
  vtkAngularPeriodicFilter.cxx
  vtkCollectGraph.cxx
  vtkCollectPolyData.cxx
  vtkCollectTable.cxx
  vtkCutMaterial.cxx
  vtkDuplicatePolyData.cxx
  vtkExtractCTHPart.cxx
  vtkExtractPolyDataPiece.cxx
  vtkExtractUnstructuredGridPiece.cxx
  vtkExtractUserDefinedPiece.cxx
  vtkPassThroughFilter.cxx
  vtkPCellDataToPointData.cxx
  vtkPeriodicFilter.cxx
  vtkPExtractDataArraysOverTime.cxx
  vtkPExtractSelectedArraysOverTime.cxx
  vtkPieceRequestFilter.cxx
  vtkPieceScalars.cxx
  vtkPipelineSize.cxx
  vtkPKdTree.cxx
  vtkPLinearExtrusionFilter.cxx
  vtkPMaskPoints.cxx
  vtkPOutlineCornerFilter.cxx
  vtkPOutlineFilter.cxx
  vtkPOutlineFilterInternals.cxx
  vtkPPolyDataNormals.cxx
  vtkPProbeFilter.cxx
  vtkPProjectSphereFilter.cxx
  vtkPReflectionFilter.cxx
  vtkPResampleFilter.cxx
  vtkProcessIdScalars.cxx
  vtkPSphereSource.cxx
  vtkPYoungsMaterialInterface.cxx
  vtkRectilinearGridOutlineFilter.cxx
  vtkRemoveGhosts.cxx
  vtkTransmitPolyDataPiece.cxx
  vtkTransmitRectilinearGridPiece.cxx
  vtkTransmitStructuredDataPiece.cxx
  vtkTransmitStructuredGridPiece.cxx
  vtkTransmitUnstructuredGridPiece.cxx
  vtkUnstructuredGridGhostCellsGenerator.cxx
  )
set_source_files_properties(
  vtkBlockDistribution.h
  HEADER_FILE_ONLY
  )

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND Module_SRCS
    vtkPExtractArraysOverTime.cxx)
endif()

vtk_module_library(vtkFiltersParallel ${Module_SRCS})
