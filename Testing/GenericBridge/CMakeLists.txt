set(module_SRCS
  vtkBridgeAttribute.cxx
  vtkBridgeCell.cxx
  vtkBridgeCellIterator.cxx
  vtkBridgeCellIteratorOnCellBoundaries.cxx
  vtkBridgeCellIteratorOnCellList.cxx
  vtkBridgeCellIteratorOnDataSet.cxx
  vtkBridgeCellIteratorOne.cxx
  vtkBridgeCellIteratorStrategy.cxx
  vtkBridgeDataSet.cxx
  vtkBridgeExport.h
  vtkBridgePointIterator.cxx
  vtkBridgePointIteratorOnCell.cxx
  vtkBridgePointIteratorOnDataSet.cxx
  vtkBridgePointIteratorOne.cxx)

# set_source_files_properties(vtkRegressionTestImage WRAP_EXCLUDE)

vtk_module_library(vtkTestingGenericBridge ${module_SRCS})
