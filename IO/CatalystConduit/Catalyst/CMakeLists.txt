if (WIN32)
  set(catalyst_library_destination
    "${_vtk_build_RUNTIME_DESTINATION}/catalyst")
else ()
  set(catalyst_library_destination
    "${_vtk_build_LIBRARY_DESTINATION}/catalyst")
  file(RELATIVE_PATH _catalyst_impl_relpath
    "/prefix/${catalyst_library_destination}"
    "/prefix/${_vtk_build_LIBRARY_DESTINATION}")
  if (APPLE)
    set(catalyst_rel_rpath
      "@loader_path/${_catalyst_impl_relpath}")
  else ()
    set(catalyst_rel_rpath
      "$ORIGIN/${_catalyst_impl_relpath}")
  endif ()

  list(APPEND CMAKE_INSTALL_RPATH
    "${catalyst_rel_rpath}")
endif ()

catalyst_implementation(
  NAME    vtk
  TARGET  catalyst-vtk
  EXPORT "${_vtk_build_INSTALL_EXPORT}"
  LIBRARY_DESTINATION
    "${catalyst_library_destination}"
  SOURCES
    VTKCatalyst.cxx
  CATALYST_TARGET catalyst::catalyst)

add_library(VTK::catalyst-vtk ALIAS catalyst-vtk)

# Clear the `-pvVERSION` suffix (if any).
set(_vtk_build_LIBRARY_NAME_SUFFIX "")
# Clear version information.
set(_vtk_build_VERSION "")
set(_vtk_build_SOVERSION "")
