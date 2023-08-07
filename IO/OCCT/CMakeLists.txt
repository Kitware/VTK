vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE OpenCASCADE
  FORWARD_VERSION_REQ EXACT
  VERSION_VAR "@OpenCASCADE_MAJOR_VERSION@.@OpenCASCADE_MINOR_VERSION@.@OpenCASCADE_MAINTENANCE_VERSION@"
)

set(opencascade_req_targets
  TKSTEP
  TKIGES
  TKMesh
  TKXDESTEP
  TKXDEIGES)
set(opencascade_missing_targets)
foreach (opencascade_req_target IN LISTS opencascade_req_targets)
  if (NOT TARGET "${opencascade_req_target}")
    list(APPEND opencascade_missing_targets
      "${opencascade_req_target}")
  endif ()
endforeach ()
if (opencascade_missing_targets)
  string(REPLACE ";" ", " opencascade_missing_targets "${opencascade_missing_targets}")
  message(FATAL_ERROR
    "Found OpenCASCADE (${OpenCASCADE_DIR}) is missing the following required "
    "targets: ${opencascade_missing_targets}")
endif ()

vtk_module_add_module(VTK::IOOCCT
  CLASSES
    vtkOCCTReader
)

vtk_module_link(VTK::IOOCCT
  PRIVATE
    ${opencascade_req_targets})

# OpenCASCADE started putting include directory usage requirements in 7.7.0.
set(OpenCASCADE_VERSION
  "${OpenCASCADE_MAJOR_VERSION}.${OpenCASCADE_MINOR_VERSION}.${OpenCASCADE_MAINTENANCE_VERSION}")
if (OpenCASCADE_VERSION VERSION_LESS "7.7.0")
  vtk_module_include(VTK::IOOCCT PRIVATE "${OpenCASCADE_INCLUDE_DIR}")
endif ()

vtk_add_test_mangling(VTK::IOOCCT)
