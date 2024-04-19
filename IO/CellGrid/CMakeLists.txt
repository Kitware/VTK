set(classes
  vtkIOCellGrid
  vtkCellGridIOQuery
  vtkCellGridReader
  vtkCellGridWriter
  vtkCompositeCellGridReader
  vtkDGIOResponder
)

vtk_module_add_module(VTK::IOCellGrid
  CLASSES ${classes}
)
