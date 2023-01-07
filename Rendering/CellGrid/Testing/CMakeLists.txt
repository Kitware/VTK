vtk_module_test_data(
  Data/dgHexahedra.dg
  Data/dgTetrahedra.dg
)

# if (NOT vtk_testing_cxx_disabled)
#   add_subdirectory(Cxx)
# endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
