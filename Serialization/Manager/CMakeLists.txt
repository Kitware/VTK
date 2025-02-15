set(classes
  vtkObjectManager)

get_property(_vtk_serialization_manager_public_deps GLOBAL
  PROPERTY "_vtk_module_VTK::SerializationManager_depends")
get_property(_vtk_serialization_manager_optional_deps GLOBAL
  PROPERTY "_vtk_module_VTK::SerializationManager_optional_depends")

set (_vtk_serdes_modules)
foreach (_vtk_serdes_module IN LISTS vtk_modules) # Comes from top-level CMakeLists.txt
  if (NOT TARGET "${_vtk_serdes_module}")
    continue ()
  endif ()
  _vtk_module_get_module_property("${_vtk_serdes_module}"
    PROPERTY  "include_marshal"
    VARIABLE  _vtk_serdes_module_include_marshal)
  if (NOT _vtk_serdes_module_include_marshal)
    continue ()
  endif ()
  if (NOT "${_vtk_serdes_module}" IN_LIST _vtk_serialization_manager_optional_deps
      AND NOT "${_vtk_serdes_module}" IN_LIST _vtk_serialization_manager_public_deps)
    message(FATAL_ERROR
      "${_vtk_serdes_module} has `INCLUDE_MARSHAL` but it is not a dependency of VTK::SerializationManager")
  endif ()
  list(APPEND _vtk_serdes_modules "${_vtk_serdes_module}")
endforeach ()

set(_vtk_serdes_registrar_source
  "${CMAKE_CURRENT_BINARY_DIR}/vtkObjectManagerDefaultRegistrar.cxx")

vtk_module_generate_libraries_serdes_registrar(
  REGISTRAR_NAME      "vtkObjectManagerDefaultSerDes"
  REGISTRAR_SOURCE    "${_vtk_serdes_registrar_source}"
  MANDATORY_MODULES   ${_vtk_serialization_manager_public_deps}
  OPTIONAL_MODULES    ${_vtk_serialization_manager_optional_deps})

vtk_module_add_module(VTK::SerializationManager
  CLASSES ${classes}
  SOURCES ${_vtk_serdes_registrar_source})

vtk_add_test_mangling(VTK::SerializationManager)
