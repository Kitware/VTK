#-----------------------------------------------------------------------------
function(fides_generate_export_header lib_name)
  # Now generate a header that holds the macros needed to easily export
  # template classes. This
  string(TOUPPER ${lib_name} BASE_NAME_UPPER)
  set(EXPORT_MACRO_NAME "${BASE_NAME_UPPER}")

  set(EXPORT_IS_BUILT_STATIC 0)
  get_target_property(is_static ${lib_name} TYPE)
  if(${is_static} STREQUAL "STATIC_LIBRARY")
    #If we are building statically set the define symbol
    set(EXPORT_IS_BUILT_STATIC 1)
  endif()
  unset(is_static)

  get_target_property(EXPORT_IMPORT_CONDITION ${lib_name} DEFINE_SYMBOL)
  if(NOT EXPORT_IMPORT_CONDITION)
    #set EXPORT_IMPORT_CONDITION to what the DEFINE_SYMBOL would be when
    #building shared
    set(EXPORT_IMPORT_CONDITION ${lib_name}_EXPORTS)
  endif()

  configure_file(
      ${FIDES_SOURCE_DIR}/cmake/FidesExportHeaderTemplate.h.in
      ${CMAKE_CURRENT_BINARY_DIR}/${lib_name}_export.h
    @ONLY)

endfunction()
