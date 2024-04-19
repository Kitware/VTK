vtk_module_test_data(
  Data/PIO/simple.pio
  Data/PIO/dumps.simple/simple-dmp000000
  Data/PIO/dumps.simple/simple-dmp000010
  Data/PIO/simple_h5.pio
  Data/PIO/dumps.simple_h5/simple-dmp000000.h5
  Data/PIO/dumps.simple_h5/simple-dmp000005.h5
)

if (VTK_WRAP_PYTHON)
  add_subdirectory(Cxx)
  add_subdirectory(Python)
endif ()
