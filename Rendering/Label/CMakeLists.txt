set(classes
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

vtk_module_add_module(VTK::RenderingLabel
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})
vtk_add_test_mangling(VTK::RenderingLabel)
