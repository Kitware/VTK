set(Module_SRCS
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
  vtkPCellDataToPointData.cxx
  vtkPExtractArraysOverTime.cxx
  vtkPeriodicFilter.cxx
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
  vtkPSphereSource.cxx
  vtkPYoungsMaterialInterface.cxx
  vtkPassThroughFilter.cxx
  vtkPieceRequestFilter.cxx
  vtkPieceScalars.cxx
  vtkPipelineSize.cxx
  vtkProcessIdScalars.cxx
  vtkRectilinearGridOutlineFilter.cxx
  vtkTransmitPolyDataPiece.cxx
  vtkTransmitStructuredDataPiece.cxx
  vtkTransmitRectilinearGridPiece.cxx
  vtkTransmitStructuredGridPiece.cxx
  vtkTransmitUnstructuredGridPiece.cxx
  )
set_source_files_properties(
  vtkBlockDistribution.h
  HEADER_FILE_ONLY
  )

set_source_files_properties(
  ${vtk-module}ObjectFactory.cxx
  vtkPOutlineFilterInternals.cxx
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

vtk_module_library(vtkFiltersParallel ${Module_SRCS})
