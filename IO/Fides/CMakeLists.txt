set(classes
  vtkFidesReader
  vtkFidesWriter)

set(VTK_HAVE_CONDUIT 0)
set(VTK_HAVE_CONDUIT_PYTHON 0)

if (TARGET VTK::conduit)
  set(VTK_HAVE_CONDUIT 1)

  if (VTK_WRAP_PYTHON AND TARGET VTK::conduit_python)
    set(VTK_HAVE_CONDUIT_PYTHON 1)
  endif()
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/fidesdefs.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/fidesdefs.h"
)

vtk_module_add_module(VTK::IOFides
  CLASSES           ${classes}
  PRIVATE_HEADERS   "${CMAKE_CURRENT_BINARY_DIR}/fidesdefs.h")

vtk_module_link(VTK::IOFides PRIVATE VTK::fides)

if (TARGET VTK::conduit)
  vtk_module_link(VTK::IOFides PRIVATE VTK::conduit)

  if (VTK_HAVE_CONDUIT_PYTHON)
    vtk_module_link(VTK::IOFides PRIVATE VTK::conduit_python)
  endif()
endif()

vtk_add_test_mangling(VTK::IOFides)

if (VTK_USE_MPI)
    vtk_module_definitions(VTK::IOFides PRIVATE IOFIDES_HAVE_MPI)
endif()
