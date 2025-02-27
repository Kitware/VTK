vtk_module_test_data(
  Data/clt.vtm
  Data/clt/,REGEX:.*
  Data/ParticleTracers/image_pathline.vtp
  Data/ParticleTracers/image_streakline.vtp
  Data/ParticleTracers/image_tracer.vtp
  Data/ParticleTracers/ug_pathline.vtp
  Data/ParticleTracers/ug_streakline.vtp
  Data/ParticleTracers/ug_tracer.vtp)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  vtk_module_test_data(
    Data/combq.bin
    Data/combxyz.bin
  )
  add_subdirectory(Python)
endif ()
