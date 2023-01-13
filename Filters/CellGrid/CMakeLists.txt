set(classes
  # Registrar
  vtkFiltersCellGrid

  # Algorithms
  vtkCellGridComputeSurface
  vtkCellGridElevation
  vtkCellGridPointProbe
  vtkUnstructuredGridToCellGrid

  # Queries
  vtkCellGridElevationQuery

  # Responders
  vtkDGBoundsResponder
  vtkDGElevationResponder
  vtkDGEvaluator
  vtkDGRangeResponder
  vtkDGSidesResponder
  vtkDGTranscribeUnstructuredCells

  # Calculator bases
  vtkInterpolateCalculator
  vtkCellAttributeInformation

  # Calculator implementations
  vtkDGInterpolateCalculator
  vtkDGAttributeInformation

  # Cell types
  vtkDGCell
  vtkDGEdge
  vtkDGHex
  vtkDGPyr
  vtkDGQuad
  vtkDGTet
  vtkDGTri
  vtkDGVert
  vtkDGWdg
  vtkDeRhamCell
)

vtk_module_add_module(VTK::FiltersCellGrid
  CLASSES ${classes}
)
