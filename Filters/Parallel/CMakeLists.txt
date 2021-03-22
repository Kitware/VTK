set(classes
  vtkAdaptiveTemporalInterpolator
  vtkAggregateDataSetFilter
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
  vtkHyperTreeGridGhostCellsGenerator
  vtkIntegrateAttributes
  vtkPassThroughFilter
  vtkPeriodicFilter
  vtkPCellDataToPointData
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
  vtkPPartitionedDataSetCollectionToMultiBlockDataSet
  vtkPPolyDataNormals
  vtkPProbeFilter
  vtkPProjectSphereFilter
  vtkPReflectionFilter
  vtkPResampleFilter
  vtkProbeLineFilter
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
  vtkTransmitUnstructuredGridPiece
  vtkUnstructuredGridGhostCellsGenerator)

set(headers
  vtkBlockDistribution.h)

vtk_object_factory_declare(
  BASE vtkPartitionedDataSetCollectionToMultiBlockDataSet
  OVERRIDE vtkPPartitionedDataSetCollectionToMultiBlockDataSet
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
