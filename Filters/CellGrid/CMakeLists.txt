set(classes
  vtkCellGridComputeSurface
  vtkDGBoundsResponder
  vtkDGCell
  vtkDGHex
  vtkDGSidesResponder
  vtkDGTet
)

vtk_module_add_module(VTK::FiltersCellGrid
  CLASSES ${classes}
)
