vtk_module_test_data(
  Data/dgHexahedra.dg
  Data/dgTetrahedra.dg
)

add_subdirectory(Cxx)

# if (VTK_WRAP_PYTHON)
#   vtk_module_test_data(
#     Data/PythonCellGrid.data
#   )
#   add_subdirectory(Python)
# endif ()
