vtk_module_test_data(
  Data/isofill_0.vtp)

add_subdirectory(Cxx)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
