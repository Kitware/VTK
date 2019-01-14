set(classes
  vtkBridgeAttribute
  vtkBridgeCell
  vtkBridgeCellIterator
  vtkBridgeCellIteratorOnCellBoundaries
  vtkBridgeCellIteratorOnCellList
  vtkBridgeCellIteratorOnDataSet
  vtkBridgeCellIteratorOne
  vtkBridgeCellIteratorStrategy
  vtkBridgeDataSet
  vtkBridgePointIterator
  vtkBridgePointIteratorOnCell
  vtkBridgePointIteratorOnDataSet
  vtkBridgePointIteratorOne)

set(headers
  vtkBridgeExport.h)

vtk_module_add_module(VTK::TestingGenericBridge
  CLASSES ${classes}
  HEADERS ${headers})
