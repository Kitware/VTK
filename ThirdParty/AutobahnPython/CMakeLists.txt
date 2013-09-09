vtk_module_impl()
vtk_module_export("")

option(VTK_USE_SYSTEM_AUTOBAHN "Use system Autobahn Python package" OFF)
mark_as_advanced(VTK_USE_SYSTEM_AUTOBAHN)

if(NOT VTK_USE_SYSTEM_AUTOBAHN)
  find_package(PythonInterp)

  include(vtkPythonPackages)

  set(AutobahnPython_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/autobahn")
  set(AutobahnPython_BINARY_DIR "${VTK_BUILD_PYTHON_MODULE_DIR}/autobahn")

  build_python_package("AutobahnPython" ${AutobahnPython_SOURCE_DIR}
    ${AutobahnPython_BINARY_DIR})

  install(DIRECTORY ${AutobahnPython_BINARY_DIR}
    DESTINATION "${VTK_INSTALL_PYTHON_MODULE_DIR}"
    COMPONENT Runtime)
endif()
