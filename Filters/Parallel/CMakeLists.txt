set(classes
  vtkAdaptiveTemporalInterpolator
  vtkAggregateDataSetFilter
  vtkAlignImageDataSetFilter
  vtkAngularPeriodicFilter
  vtkCollectGraph
  vtkCollectPolyData
  vtkCollectTable
  vtkCutMaterial
  vtkDistributedDataFilter
  vtkDuplicatePolyData
  vtkExtractCTHPart
  vtkExtractPolyDataPiece
  vtkExtractUnstructuredGridPiece
  vtkExtractUserDefinedPiece
  vtkGenerateProcessIds
  vtkHyperTreeGridGhostCellsGenerator
  vtkPHyperTreeGridProbeFilter
  vtkIntegrateAttributes
  vtkPeriodicFilter
  vtkPCellDataToPointData
  vtkPConvertToMultiBlockDataSet
  vtkPExtractDataArraysOverTime
  vtkPExtractExodusGlobalTemporalVariables
  vtkPExtractSelectedArraysOverTime
  vtkPieceRequestFilter
  vtkPieceScalars
  vtkPipelineSize
  vtkPKdTree
  vtkPLinearExtrusionFilter
  vtkPMaskPoints
  vtkPMergeArrays
  vtkPOutlineCornerFilter
  vtkPOutlineFilter
  vtkPOutlineFilterInternals
  vtkPPolyDataNormals
  vtkPProbeFilter
  vtkPProjectSphereFilter
  vtkPReflectionFilter
  vtkPResampleFilter
  vtkPartitionBalancer
  vtkProcessIdScalars
  vtkPSphereSource
  vtkPTextureMapToSphere
  vtkPYoungsMaterialInterface
  vtkRectilinearGridOutlineFilter
  vtkRemoveGhosts
  vtkTransmitPolyDataPiece
  vtkTransmitRectilinearGridPiece
  vtkTransmitStructuredDataPiece
  vtkTransmitStructuredGridPiece
  vtkTransmitUnstructuredGridPiece)

set(headers
  vtkBlockDistribution.h)

vtk_object_factory_declare(
  BASE vtkConvertToMultiBlockDataSet
  OVERRIDE vtkPConvertToMultiBlockDataSet
  )

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKFILTERSPARALLEL_EXPORT"
  )

vtk_module_add_module(VTK::FiltersParallel
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${vtk_object_factory_source}
  PRIVATE_HEADERS ${vtk_object_factory_header}
  )
vtk_add_test_mangling(VTK::FiltersParallel)
