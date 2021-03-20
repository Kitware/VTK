set(classes
  QQmlVTKPlugin
  QQuickVTKInteractorAdapter
  QQuickVTKRenderItem
  QQuickVTKRenderWindow
  )

include(vtkQt)
set(qt_components
  Gui
  OpenGL
  Quick
  Qml
  )

vtk_module_find_package(
  PACKAGE     "Qt${vtk_qt_major_version}"
  VERSION     5.9
  COMPONENTS  ${qt_components}
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         "Qt${vtk_qt_major_version}_VERSION")

set(CMAKE_AUTOMOC 1)

vtk_module_add_module(VTK::GUISupportQtQuick
  CLASSES ${classes}
  )

foreach(_qt_comp IN LISTS qt_components)
  list(APPEND qt_modules "Qt${vtk_qt_major_version}::${_qt_comp}")
endforeach()

vtk_module_definitions(VTK::GUISupportQtQuick PRIVATE QT_NO_KEYWORDS)
vtk_module_link(VTK::GUISupportQtQuick PUBLIC ${qt_modules})

_vtk_module_real_target(vtk_guisupportqtquick_target VTK::GUISupportQtQuick)

# Generate the qmldir file for module plugin consumption in a Qml project

# QML plugin qmldir and qmltypes for build tree
# Set VTK_MODULE_PLUGIN_NAME and VTK_MODULE_PLUGIN_DIR for the qmldir configuration
set(vtk_qml_module_dir
  ${_vtk_build_PACKAGE}.${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION})
set(vtk_qml_module_path
  ${CMAKE_BINARY_DIR}/$<CONFIG>/${vtk_qml_module_dir})
# set(vtk_qml_module_path
#   ${CMAKE_BINARY_DIR}/${vtk_qml_module_dir})

# We use a three step approach to resolving the QML plugin path for qmldir file
# during compilation of this module.
# The first step would be to configure the qmldir.in file to a pre-generator
# file. This step would be followed by a generator step that fills in the right
# directory location for VTK and generates a qmldir file for each
# possible build configuration i.e. Debug, Release, etc.
# The third step is to copy the right configuration file to the plugin
# location at build time. This is done as a custom command.
set(VTK_MODULE_PLUGIN_NAME "$<TARGET_FILE_BASE_NAME:${vtk_guisupportqtquick_target}>")
set(VTK_MODULE_PLUGIN_DIR "$<TARGET_FILE_DIR:${vtk_guisupportqtquick_target}>")
set(vtk_qml_module_tmp_build_path
  ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/qmldirBuild
  )
set(qmldir_pregen_file
  "${vtk_qml_module_tmp_build_path}/qmldir.pregen")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/qmldir.in
  ${qmldir_pregen_file}
  @ONLY
  )
set(qmldir_gen_file
  "${vtk_qml_module_tmp_build_path}/qmldir$<CONFIG>.gen")
file(GENERATE OUTPUT ${qmldir_gen_file}
  INPUT ${qmldir_pregen_file})
add_custom_command(TARGET ${vtk_guisupportqtquick_target}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${qmldir_gen_file} ${vtk_qml_module_path}/qmldir
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  # Commented out because generator expreesions not supported in CMake < 3.20 for byproducts
  # BYPRODUCTS ${vtk_qml_module_path}/qmldir
  COMMENT "Generating qmldir file"
  VERBATIM
  )

# Generate the qmltypes file for the VTK Qml plugin
# First, find the qmlplugindump executable
get_target_property(qt_core_location Qt${vtk_qt_major_version}::Core LOCATION)
get_filename_component(qt_bin_dir "${qt_core_location}" PATH)
if(APPLE)
  get_filename_component(qt_bin_dir "${qt_bin_dir}" PATH)
endif()
find_program(QMLPLUGINDUMP_EXECUTABLE qmlplugindump
  HINTS "${qt_bin_dir}"
  DOC "QmlPlugindump executable location"
  )
mark_as_advanced(QMLPLUGINDUMP_EXECUTABLE)
if(NOT QMLPLUGINDUMP_EXECUTABLE)
  message(STATUS "qmlplugindump executable not found.\n"
                  "It is required to generate the qmltypes file"
                  " for VTK Qml plugin.")
endif()

set(qmltypes_file
  "${vtk_qml_module_path}/${_vtk_build_PACKAGE}.qmltypes"
  )
if(QMLPLUGINDUMP_EXECUTABLE)
  get_target_property(qt_config Qt${vtk_qt_major_version}::Core IMPORTED_CONFIGURATIONS)
  if(("DEBUG" IN_LIST qt_config) AND ("RELEASE" IN_LIST qt_config))
    MESSAGE (WARNING "Qt5 is configured in both Debug and Release modes."
      " Due to Qt issue 47774 (https://bugreports.qt.io/browse/QTBUG-47774),"
      " skipping generation of qmltypes file."
      " Using the one provided with the source tree instead.")
    add_custom_command(TARGET ${vtk_guisupportqtquick_target}
      POST_BUILD
      COMMAND
        ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${_vtk_build_PACKAGE}.qmltypes
          ${vtk_qml_module_path}
      )
    # file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${_vtk_build_PACKAGE}.qmltypes
    #   DESTINATION ${vtk_qml_module_path}
    #   )
  else()
    add_custom_command(TARGET ${vtk_guisupportqtquick_target}
      POST_BUILD
      COMMAND
        ${QMLPLUGINDUMP_EXECUTABLE} ${_vtk_build_PACKAGE}
        ${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}
        ${CMAKE_BINARY_DIR} > ${qmltypes_file}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      # BYPRODUCTS ${qmltypes_file}
      COMMENT "Generating qmltypes file using qmlplugindump"
      VERBATIM
      )
  endif()
endif()

# QML plugin qmldir and qmltypes for install tree
set(vtk_qml_module_tmp_install_path
  ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/qmldirInstall
  )
set(vtk_qml_module_install_path
  ${vtk_qml_module_tmp_install_path}/$<CONFIG>/${vtk_qml_module_dir})
if(WIN32 AND NOT CYGWIN)
  # Set VTK_MODULE_PLUGIN_DIR for the qmldir configuration
  # Use the runtime (dll) path for plugin on Windows
  file(RELATIVE_PATH VTK_MODULE_PLUGIN_DIR
    ${CMAKE_INSTALL_PREFIX}/${_vtk_build_PACKAGE}
    ${CMAKE_INSTALL_PREFIX}/${_vtk_build_RUNTIME_DESTINATION}
    )
else()
  # Set VTK_MODULE_PLUGIN_DIR for the qmldir configuration
  file(RELATIVE_PATH VTK_MODULE_PLUGIN_DIR
    ${CMAKE_INSTALL_PREFIX}/${_vtk_build_PACKAGE}
    ${CMAKE_INSTALL_PREFIX}/${_vtk_build_LIBRARY_DESTINATION}
    )
endif()
set(qmldir_install_pregen_file
  "${vtk_qml_module_tmp_install_path}/qmldir.install.pregen")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/qmldir.in"
  "${qmldir_install_pregen_file}"
  @ONLY
  )
set(qmldir_install_gen_file
  "${vtk_qml_module_install_path}/qmldir")
file(GENERATE OUTPUT ${qmldir_install_gen_file}
  INPUT ${qmldir_install_pregen_file})

install(FILES ${qmltypes_file}
  DESTINATION ${vtk_qml_module_install_path}
  COMPONENT ${_vtk_build_TARGETS_COMPONENT}
  )

# Install the QML plugin module
install(DIRECTORY
  ${vtk_qml_module_install_path}
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
  COMPONENT ${_vtk_build_TARGETS_COMPONENT}
  )
