set(classes
  vtkObjectManager)

set(nowrap_classes
  vtkSession)

get_property(_vtk_serialization_manager_public_deps GLOBAL
  PROPERTY "_vtk_module_VTK::SerializationManager_depends")
get_property(_vtk_serialization_manager_optional_deps GLOBAL
  PROPERTY "_vtk_module_VTK::SerializationManager_optional_depends")

set(_vtk_serdes_registrar_source
  "${CMAKE_CURRENT_BINARY_DIR}/vtkObjectManagerDefaultRegistrar.cxx")

vtk_module_generate_libraries_serdes_registrar(
  REGISTRAR_NAME      "vtkObjectManagerDefaultSerDes"
  REGISTRAR_SOURCE    "${_vtk_serdes_registrar_source}"
  MANDATORY_MODULES   ${_vtk_serialization_manager_public_deps}
  OPTIONAL_MODULES    ${_vtk_serialization_manager_optional_deps})

vtk_module_add_module(VTK::SerializationManager
  CLASSES ${classes}
  NOWRAP_CLASSES ${nowrap_classes}
  SOURCES ${_vtk_serdes_registrar_source})

vtk_add_test_mangling(VTK::SerializationManager)
