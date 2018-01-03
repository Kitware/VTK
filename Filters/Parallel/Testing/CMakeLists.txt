vtk_module_test_data(
  Data/RectGrid2.vtk
  Data/SampleStructGrid.vtk
  Data/ironProt.vtk
  Data/periodicPiece.vtu
  Data/tetraMesh.vtk)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/RectGrid2.vtr
    Data/cth.vtr
    Data/elements.vtu
    Data/headsq/,REGEX:.*)

  add_subdirectory(Python)
endif ()
