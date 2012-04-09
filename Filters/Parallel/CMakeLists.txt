find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

set(Module_SRCS
  vtkCollectGraph.cxx
  vtkCollectPolyData.cxx
  vtkCollectTable.cxx
  vtkCachingInterpolatedVelocityField.cxx
  vtkCutMaterial.cxx
  vtkDistributedDataFilter.cxx
  # vtkDistributedStreamTracer.cxx # needs vtkStreamTracer
  vtkDuplicatePolyData.cxx
  vtkExtractCTHPart.cxx
  # vtkExtractPiece.cxx # needs vtkImageClip
  vtkExtractPolyDataPiece.cxx
  vtkExtractUnstructuredGridPiece.cxx
  vtkExtractUserDefinedPiece.cxx
  # vtkImageRenderManager.cxx # needs to move
  # vtkMemoryLimitImageDataStreamer.cxx # nees vtkImageDataStreamer
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
  # vtkPStreamTracer.cxx # needs vtkStreamTracer
  vtkPTableToStructuredGrid.cxx
  vtkRectilinearGridOutlineFilter.cxx
  # vtkTemporalFractal.cxx # needs vtkImageMandelbrotSource
  vtkTemporalInterpolatedVelocityField.cxx
  # vtkTemporalStreamTracer.cxx # needs vtkStreamTracer
  # vtkTransmitImageDataPiece.cxx # needs vtkImageClip
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
