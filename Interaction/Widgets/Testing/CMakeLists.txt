vtk_module_test_data(
  Data/AngularSector.vtk
  Data/EventRecording.log
  Data/SainteHelens.dem
  Data/SyntheticPolyline.vtp
  Data/beach.tif
  Data/combq.bin
  Data/combxyz.bin
  Data/fran_cut.png
  Data/headsq/,REGEX:.*
  )

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
