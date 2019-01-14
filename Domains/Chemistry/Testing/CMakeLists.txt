vtk_module_test_data(
  Data/2LYZ.pdb
  Data/2h2o.aux
  Data/3GQP.pdb
  Data/porphyrin.cml)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/nanowireTB23K298.xyz)

  add_subdirectory(Python)
endif ()
