set(classes
  # Registrar
  vtkFiltersCellGrid

  # Algorithms
  vtkCellGridComputeSides
  vtkCellGridElevation
  vtkCellGridPointProbe
  vtkUnstructuredGridToCellGrid

  # Queries
  vtkCellGridElevationQuery

  # Responders
  vtkDGBoundsResponder
  vtkDGCopyResponder
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

  # Numerics and low-level classes
  vtkDGConstantOperators
  vtkDGHGradOperators
  vtkDGHCurlOperators
  vtkDGHDivOperators
  vtkDGOperatorEntry
  vtkDGInvokeOperator
)

# Encode the basis implementations as strings we can
# pass as shader code.
set(basis_files
  Basis/HGrad/TetC1Basis.h
  Basis/HGrad/HexC2Gradient.h
  Basis/HGrad/HexC1Basis.h
  Basis/HGrad/WdgI2Gradient.h
  Basis/HGrad/QuadC1Gradient.h
  Basis/HGrad/HexI2Basis.h
  Basis/HGrad/TriC1Basis.h
  Basis/HGrad/PyrF2Basis.h
  Basis/HGrad/QuadC2Basis.h
  Basis/HGrad/PyrF2Gradient.h
  Basis/HGrad/PyrC1Gradient.h
  Basis/HGrad/WdgF2Basis.h
  Basis/HGrad/WdgC2Gradient.h
  Basis/HGrad/PyrI2Gradient.h
  Basis/HGrad/TetC1Gradient.h
  Basis/HGrad/PyrC1Basis.h
  Basis/HGrad/TetF2Gradient.h
  Basis/HGrad/WdgC1Gradient.h
  Basis/HGrad/EdgeC1Gradient.h
  Basis/HGrad/TetF2Basis.h
  Basis/HGrad/TetC2Gradient.h
  Basis/HGrad/PyrI2Basis.h
  Basis/HGrad/HexC1Gradient.h
  Basis/HGrad/TriC2Basis.h
  Basis/HGrad/TetC2Basis.h
  Basis/HGrad/EdgeC1Basis.h
  Basis/HGrad/QuadC1Basis.h
  Basis/HGrad/TriC1Gradient.h
  Basis/HGrad/EdgeC2Gradient.h
  Basis/HGrad/WdgI2Basis.h
  Basis/HGrad/HexC2Basis.h
  Basis/HGrad/QuadC2Gradient.h
  Basis/HGrad/WdgC2Basis.h
  Basis/HGrad/HexI2Gradient.h
  Basis/HGrad/PyrC2Gradient.h
  Basis/HGrad/PyrC2Basis.h
  Basis/HGrad/EdgeC2Basis.h
  Basis/HGrad/TriC2Gradient.h
  Basis/HGrad/WdgF2Gradient.h
  Basis/HGrad/WdgC1Basis.h
  Basis/HDiv/QuadI1Basis.h
  Basis/HDiv/TriI1Basis.h
  Basis/HDiv/HexI1Basis.h
  Basis/HDiv/WdgI1Basis.h
  Basis/HDiv/TetI1Basis.h
  Basis/HCurl/QuadI1Basis.h
  Basis/HCurl/TriI1Basis.h
  Basis/HCurl/HexI1Basis.h
  Basis/HCurl/WdgI1Basis.h
  Basis/HCurl/TetI1Basis.h
  Basis/Constant/CellC0Basis.h
  Basis/Constant/CellC0Gradient.h
)
foreach(file IN LISTS basis_files)
  get_filename_component(basis_name "${file}" NAME_WE)
  get_filename_component(basis_path "${file}" DIRECTORY)
  string(REPLACE "/" "_" basis_var "${basis_path}/${basis_name}")
  vtk_encode_string(
    INPUT         "${file}"
    NAME          "${basis_var}"
    EXPORT_SYMBOL "VTKFILTERSCELLGRID_NO_EXPORT"
    EXPORT_HEADER "vtkFiltersCellGridModule.h"
    HEADER_OUTPUT header
    SOURCE_OUTPUT source)
  list(APPEND sources
    "${source}")
  list(APPEND private_headers
    "${header}")
endforeach()


vtk_module_add_module(VTK::FiltersCellGrid
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers}
)
