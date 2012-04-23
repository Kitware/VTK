set(Module_SRCS
  vtkLabeledDataMapper.cxx
  vtkLabeledTreeMapDataMapper.cxx
  vtkLabelHierarchyAlgorithm.cxx
  vtkLabelHierarchyCompositeIterator.cxx
  vtkLabelHierarchy.cxx
  vtkLabelHierarchyIterator.cxx
  vtkLabelPlacementMapper.cxx
  vtkLabelPlacer.cxx
  vtkLabelRenderStrategy.cxx
  vtkLabelSizeCalculator.cxx
  vtkFreeTypeLabelRenderStrategy.cxx
  vtkDynamic2DLabelMapper.cxx
  vtkPointSetToLabelHierarchy.cxx
  )

set_source_files_properties(
  vtkLabelHierarchyIterator
  vtkLabelRenderStrategy
  ABSTRACT
  )

include_directories(${VTK_SOURCE_DIR}/Utilities) # octree
vtk_module_library(vtkRenderingLabel ${Module_SRCS})
