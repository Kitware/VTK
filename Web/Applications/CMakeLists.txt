vtk_module_impl()
vtk_module_export("")

find_package(PythonInterp ${VTK_PYTHON_VERSION} REQUIRED)

set(WEB_APPLICATIONS
  Cone
  GraphLayout
  PhylogeneticTree
  FileBrowser
)

set(WEB_APPS_DEPENDS)

include(vtkPythonPackages) # for copy_files_recursive
foreach(_app ${WEB_APPLICATIONS})
  file(MAKE_DIRECTORY "${VTK_WWW_DIR}/apps/${_app}")

  copy_files_recursive(${CMAKE_CURRENT_SOURCE_DIR}/${_app}/www
    DESTINATION ${VTK_WWW_DIR}/apps/${_app}
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/www-${_app}-complete"
    REGEX "^(.*\\.*)$"
    LABEL "Copying Web files for ${_app}")

  copy_files_recursive(${CMAKE_CURRENT_SOURCE_DIR}/${_app}/server
    DESTINATION "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/../Wrapping/Python/vtk/web"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/py-${_app}-complete"
    REGEX "^(.*\\.py)$"
    LABEL "Copying Server files for ${_app}")

  list(APPEND WEB_APPS_DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/py-${_app}-complete"
    "${CMAKE_CURRENT_BINARY_DIR}/www-${_app}-complete")

endforeach()

add_custom_target(vtkWebApps ALL DEPENDS
  ${WEB_APPS_DEPENDS}
)
