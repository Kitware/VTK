set(classes
  vtkBatchedLabeledDataMapper
  vtkDynamic2DLabelMapper
  vtkFreeTypeLabelRenderStrategy
  vtkLabeledDataMapper
  vtkLabeledTreeMapDataMapper
  vtkLabelHierarchy
  vtkLabelHierarchyAlgorithm
  vtkLabelHierarchyCompositeIterator
  vtkLabelHierarchyIterator
  vtkLabelPlacementMapper
  vtkLabelPlacer
  vtkLabelRenderStrategy
  vtkLabelSizeCalculator
  vtkPointSetToLabelHierarchy)

set(private_headers
  Private/vtkLabeledFormatter.h)

set(serialization_helper_sources)
if (VTK_WRAP_SERIALIZATION)
  list(APPEND serialization_helper_sources
    vtkBatchedLabeledDataMapperSerDesHelper.cxx)
endif ()

vtk_module_add_module(VTK::RenderingLabel
  CLASSES ${classes}
  SOURCES ${serialization_helper_sources}
  PRIVATE_HEADERS ${private_headers})
vtk_add_test_mangling(VTK::RenderingLabel)
