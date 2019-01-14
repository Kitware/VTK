set(classes
  vtkAggregateDataSetFilter
  vtkAngularPeriodicFilter
  vtkCollectGraph
  vtkCollectPolyData
  vtkCollectTable
  vtkCutMaterial
  vtkDuplicatePolyData
  vtkExtractCTHPart
  vtkExtractPolyDataPiece
  vtkExtractUnstructuredGridPiece
  vtkExtractUserDefinedPiece
  vtkIntegrateAttributes
  vtkPCellDataToPointData
  vtkPExtractDataArraysOverTime
  vtkPExtractSelectedArraysOverTime
  vtkPeriodicFilter
  vtkPKdTree
  vtkPLinearExtrusionFilter
  vtkPMaskPoints
  vtkPOutlineCornerFilter
  vtkPOutlineFilter
  vtkPOutlineFilterInternals
  vtkPPolyDataNormals
  vtkPProbeFilter
  vtkPProjectSphereFilter
  vtkPReflectionFilter
  vtkPResampleFilter
  vtkPSphereSource
  vtkPYoungsMaterialInterface
  vtkPassThroughFilter
  vtkPieceRequestFilter
  vtkPieceScalars
  vtkPipelineSize
  vtkProcessIdScalars
  vtkRectilinearGridOutlineFilter
  vtkRemoveGhosts
  vtkTransmitPolyDataPiece
  vtkTransmitStructuredDataPiece
  vtkTransmitRectilinearGridPiece
  vtkTransmitStructuredGridPiece
  vtkTransmitUnstructuredGridPiece
  vtkUnstructuredGridGhostCellsGenerator)

set(headers
  vtkBlockDistribution.h)

if (NOT VTK_LEGACY_REMOVE)
  list(APPEND classes
    vtkPExtractArraysOverTime)
endif ()

vtk_module_add_module(VTK::FiltersParallel
  CLASSES ${classes}
  HEADERS ${headers})
