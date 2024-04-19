vtk_module_test_data(
  Data/NE2_ps_bath_small.jpg
  Data/political.vtp
  Data/usa_image.jpg)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
