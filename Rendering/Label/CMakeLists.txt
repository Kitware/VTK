set(classes
  vtkDynamic2DLabelMapper
  vtkFreeTypeLabelRenderStrategy
  vtkLabeledDataMapper
  vtkLabeledTreeMapDataMapper
  vtkLabelHierarchyAlgorithm
  vtkLabelHierarchyCompositeIterator
  vtkLabelHierarchy
  vtkLabelHierarchyIterator
  vtkLabelPlacementMapper
  vtkLabelPlacer
  vtkLabelRenderStrategy
  vtkLabelSizeCalculator
  vtkPointSetToLabelHierarchy)

vtk_module_add_module(VTK::RenderingLabel
  CLASSES ${classes})
