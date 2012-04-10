find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(Module_SRCS
  vtkCollectGraph.cxx
  vtkCollectPolyData.cxx
  vtkCollectTable.cxx
  vtkCachingInterpolatedVelocityField.cxx
  vtkCutMaterial.cxx
  vtkDistributedDataFilter.cxx
  vtkDuplicatePolyData.cxx
  vtkExtractCTHPart.cxx
  vtkExtractPolyDataPiece.cxx
  vtkExtractUnstructuredGridPiece.cxx
  vtkExtractUserDefinedPiece.cxx
  vtkPassThroughFilter.cxx
  vtkPCellDataToPointData.cxx
  vtkPCosmoHaloFinder.cxx
  vtkPExtractArraysOverTime.cxx
  vtkPieceRequestFilter.cxx
  vtkPieceScalars.cxx
  vtkPipelineSize.cxx
  vtkPKdTree.cxx
  vtkPLinearExtrusionFilter.cxx
  vtkPOutlineCornerFilter.cxx
  vtkPOutlineFilter.cxx
  vtkPPolyDataNormals.cxx
  vtkPProbeFilter.cxx
  vtkPReflectionFilter.cxx
  vtkProcessIdScalars.cxx
  vtkPSphereSource.cxx
  vtkPTableToStructuredGrid.cxx
  vtkRectilinearGridOutlineFilter.cxx
  vtkTemporalInterpolatedVelocityField.cxx
  vtkTransmitPolyDataPiece.cxx
  vtkTransmitRectilinearGridPiece.cxx
  vtkTransmitStructuredGridPiece.cxx
  vtkTransmitUnstructuredGridPiece.cxx
  vtkPYoungsMaterialInterface.cxx
  vtkPMaskPoints.cxx
  )
set_source_files_properties(
  vtkBlockDistribution.h
  HEADER_FILE_ONLY
  )

vtk_module_library(vtkFiltersParallel ${Module_SRCS})
