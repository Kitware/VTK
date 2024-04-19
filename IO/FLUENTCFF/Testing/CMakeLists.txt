if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/room.cas
    Data/room.dat
    Data/room.dat.h5
    Data/room.cas.h5)

  add_subdirectory(Python)
endif ()
