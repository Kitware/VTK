vtk_module_test_data(
  Data/PIO/simple.pio
  Data/PIO/dumps.simple/simple-dmp000000
  Data/PIO/dumps.simple/simple-dmp000010
)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
