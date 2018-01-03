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

# TODO: need to convert octree to module.
target_include_directories(vtkRenderingLabel
  PRIVATE
    ${VTK_SOURCE_DIR}/Utilities) # octree
