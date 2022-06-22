set(config_subdir "")
get_property(multiconfig GLOBAL
  PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (multiconfig)
  set(config_subdir "/$<CONFIG>")
endif ()

set(qml_module_dir
  "${_vtk_build_PACKAGE}.${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
set(qml_build_output_dir
  "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_QMLDIR}${config_subdir}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  "${qml_build_output_dir}/${qml_module_dir}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
  "${qml_build_output_dir}/${qml_module_dir}")

# Set the VTK_QML_DIR for use in the test suite during the build. Other
# consumers will get it from `vtk-config.cmake`.
set(VTK_QML_DIR "${qml_build_output_dir}"
  CACHE INTERNAL "QML output directory")

if (multiconfig)
  foreach (config IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${config}" upper_config)
    set("CMAKE_RUNTIME_OUTPUT_DIRECTORY_${upper_config}"
      "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_QMLDIR}/${config}/${qml_module_dir}")
    set("CMAKE_LIBRARY_OUTPUT_DIRECTORY_${upper_config}"
      "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_QMLDIR}/${config}/${qml_module_dir}")
  endforeach ()
endif ()

add_library(qmlvtkplugin MODULE
  QQmlVTKPlugin.cxx
  QQmlVTKPlugin.h)
add_library(VTK::qmlvtkplugin ALIAS qmlvtkplugin)
target_link_libraries(qmlvtkplugin
  PRIVATE
    VTK::GUISupportQtQuick
    "Qt${vtk_qt_major_version}::Qml")

install(
  TARGETS qmlvtkplugin
  EXPORT  "${_vtk_build_INSTALL_EXPORT}"
  RUNTIME
    DESTINATION "${CMAKE_INSTALL_QMLDIR}${config_subdir}/${qml_module_dir}"
    COMPONENT "${_vtk_build_TARGETS_COMPONENT}"
  LIBRARY
    DESTINATION "${CMAKE_INSTALL_QMLDIR}${config_subdir}/${qml_module_dir}"
    COMPONENT "${_vtk_build_TARGETS_COMPONENT}")

# Prepare the `qmldir` file.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/qmldir.in"
  "${CMAKE_CURRENT_BINARY_DIR}/qmldir"
  @ONLY)
file(GENERATE
  OUTPUT "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/qmldir"
  INPUT  "${CMAKE_CURRENT_BINARY_DIR}/qmldir")

# Generate the qmltypes file for the VTK QML plugin

# First, find the qmlplugindump executable
get_target_property(qt_qmake_location "Qt${vtk_qt_major_version}::qmake" LOCATION)
get_filename_component(qt_bin_dir "${qt_qmake_location}" PATH)
if (APPLE)
  get_filename_component(qt_bin_dir "${qt_bin_dir}" PATH)
endif ()
find_program(QMLPLUGINDUMP_EXECUTABLE
  NAMES "qmlplugindump-qt${vtk_qt_major_version}" qmlplugindump
  HINTS "${qt_bin_dir}"
  DOC "QmlPlugindump executable location")
mark_as_advanced(QMLPLUGINDUMP_EXECUTABLE)
if (NOT QMLPLUGINDUMP_EXECUTABLE)
  message(FATAL_ERROR
    "qmlplugindump executable not found.\nIt is required to generate the "
    "qmltypes file for VTK Qml plugin.")
endif ()

set(qmltypes_file_template
  "${CMAKE_CURRENT_SOURCE_DIR}/plugins.qmltypes")
set(qmltypes_file_output
  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/plugins.qmltypes")

set(byproducts
  BYPRODUCTS "${qmltypes_file_output}")
# Generator expressions are not supported in CMake < 3.20 for byproducts
if (config_subdir AND CMAKE_VERSION VERSION_LESS "3.20")
  set(byproducts)
endif ()

get_target_property(qt_config "Qt${vtk_qt_major_version}::Core" IMPORTED_CONFIGURATIONS)

set(copy_qmlplugintypes FALSE)
if (vtk_qt_major_version VERSION_GREATER 5)
  set(copy_qmlplugintypes TRUE)
endif ()

if (NOT copy_qmlplugintypes)
  if (WIN32 AND ("DEBUG" IN_LIST qt_config) AND ("RELEASE" IN_LIST qt_config))
    if (NOT VTK_DISABLE_QT_MULTICONFIG_WINDOWS_WARNING)
      message(AUTHOR_WARNING
        "Qt5 is configured in both Debug and Release modes. Due to Qt issue "
        "47774 (https://bugreports.qt.io/browse/QTBUG-47774), skipping "
        "generation of qmltypes file. Using the one provided with the source "
        "tree instead.")
    endif ()
    set(copy_qmlplugintypes TRUE)
  else ()
    add_custom_command(
      TARGET qmlvtkplugin
      POST_BUILD
      COMMAND
        "${QMLPLUGINDUMP_EXECUTABLE}"
        -output "${qmltypes_file_output}"
        "${_vtk_build_PACKAGE}"
        "${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}"
        "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_QMLDIR}${config_subdir}"
      WORKING_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}"
      ${byproducts}
      COMMENT "Generating qmltypes file using qmlplugindump"
      VERBATIM)
  endif ()
endif()
if (copy_qmlplugintypes)
  add_custom_command(
    TARGET qmlvtkplugin
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" -E copy_if_different
      "${qmltypes_file_template}"
      "${qmltypes_file_output}"
    WORKING_DIRECTORY
      "${CMAKE_CURRENT_BINARY_DIR}"
    ${byproducts}
    COMMENT "Copying qmltypes file from the source tree to ${qmltypes_file_output}")
endif ()

# Install the QML plugin module
install(
  FILES
    "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/qmldir"
    "${qmltypes_file_output}"
  DESTINATION "${CMAKE_INSTALL_QMLDIR}${config_subdir}/${qml_module_dir}"
  COMPONENT "${_vtk_build_TARGETS_COMPONENT}")
