vtk_module_test_data(
  Data/WineGlass.wrl)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/bot2.wrl)

  add_subdirectory(Python)
endif ()
