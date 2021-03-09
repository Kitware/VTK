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

vtk_module_add_module(VTK::RenderingLabel
  CLASSES ${classes})
