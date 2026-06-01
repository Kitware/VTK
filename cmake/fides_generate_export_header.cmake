include(GenerateExportHeader)

#-----------------------------------------------------------------------------
function(fides_generate_export_header lib_name)
  string(TOUPPER ${lib_name} BASE_NAME_UPPER)
  set(export_file "${CMAKE_CURRENT_BINARY_DIR}/${lib_name}_export.h")

  # generate an export header
  generate_export_header(${lib_name}
    EXPORT_FILE_NAME "${export_file}"
    BASE_NAME ${BASE_NAME_UPPER}
    DEPRECATED_MACRO_NAME "${BASE_NAME_UPPER}_EXPORT_DEPRECATED"
  )

  # add export header to the target sources so cmake knows about it
  target_sources(${lib_name} PUBLIC
    "$<BUILD_INTERFACE:${export_file}>"
  )

  # make sure it will be found by us and consumers (installed below)
  target_include_directories(${lib_name} PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:include>"
  )

  # handle when we build a static library
  get_target_property(type ${lib_name} TYPE)
  if(type STREQUAL "STATIC_LIBRARY")
    target_compile_definitions(${lib_name} PUBLIC ${BASE_NAME_UPPER}_STATIC_DEFINE)
  endif()

  # install generated header
  install(FILES "${export_file}"
    DESTINATION ${FIDES_INSTALL_INCLUDE_DIR}
    COMPONENT Development
  )
endfunction()
