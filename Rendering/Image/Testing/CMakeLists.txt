vtk_module_test_data(
  Data/alphachannel.png
  Data/beach.tif
  Data/fullhead15.png
  Data/headsq/,REGEX:.*
  Data/vtk.png)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
