set(classes
  vtkFidesReader
  vtkFidesWriter)

vtk_module_add_module(VTK::IOFides
  CLASSES           ${classes})

vtk_module_link(VTK::IOFides PRIVATE VTK::fides)
vtk_add_test_mangling(VTK::IOFides)

if (VTK_USE_MPI)
    vtk_module_definitions(VTK::IOFides PRIVATE IOFIDES_HAVE_MPI)
endif()
