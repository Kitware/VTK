# Enforce compatibility if Catalyst is also enabled
if (TARGET VTK::catalyst)
  vtk_module_get_property(VTK::catalyst
    PROPERTY CATALYST_WITH_EXTERNAL_CONDUIT
    VARIABLE catalyst_ext_conduit
  )
  if (NOT catalyst_ext_conduit)
    message(FATAL_ERROR
      "Conflict detected: VTK::conduit is enabled, but VTK::catalyst is "
      "configured to use its internal mangled Conduit. Please reconfigure "
      "with -DCATALYST_WITH_EXTERNAL_CONDUIT=ON so they can safely share "
      "the external Conduit library."
    )
  endif()
endif()

vtk_module_third_party_external(
  PACKAGE Conduit
  TARGETS conduit::conduit
)

# INTERFACE/ALIAS workaround for the optional Python component
if (TARGET conduit::conduit_python)
  # Create a VTK-owned interface target
  add_library(vtk_conduit_python_interface INTERFACE)

  set_target_properties(vtk_conduit_python_interface PROPERTIES
    EXPORT_NAME conduit_python
  )

  # If Conduit exposed the python modules directory, we could avoid this hack
  get_target_property(_conduit_inc_dirs conduit::conduit INTERFACE_INCLUDE_DIRECTORIES)
  find_path(_conduit_py_dir
    NAMES conduit/__init__.py
    HINTS ${_conduit_inc_dirs}
    PATH_SUFFIXES "../python-modules" "../../python-modules"
    NO_DEFAULT_PATH
  )

  # If found, attach it to the target. Otherwise warn.
  if (_conduit_py_dir)
    set_target_properties(vtk_conduit_python_interface PROPERTIES
      VTK_CONDUIT_PYTHON_MODULE_DIR "${_conduit_py_dir}"
    )
  else()
    message(WARNING "Conduit Python module directory could not be located.")
  endif()

  # Link the actual Conduit python target to the VTK's interface target
  target_link_libraries(vtk_conduit_python_interface INTERFACE conduit::conduit_python)

  # Make it visible to the test directories
  add_library(VTK::conduit_python ALIAS vtk_conduit_python_interface)

  # Export the interface target during installation
  install(TARGETS vtk_conduit_python_interface EXPORT "${_vtk_build_INSTALL_EXPORT}")
endif()
