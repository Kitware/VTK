option("${PROJECT_NAME}_DEBUG_MODULE" "Debug module logic in ${PROJECT_NAME}" OFF)
mark_as_advanced("${PROJECT_NAME}_DEBUG_MODULE")

set(_vtk_module_log)

include(CMakeDependentOption)
cmake_dependent_option("${PROJECT_NAME}_DEBUG_MODULE_ALL" "Enable all debugging" OFF
  "${PROJECT_NAME}_DEBUG_MODULE" OFF)
mark_as_advanced("${PROJECT_NAME}_DEBUG_MODULE_ALL")

if (${PROJECT_NAME}_DEBUG_MODULE_ALL)
  set(_vtk_module_log "ALL")
else ()
  set(_builtin_domains
    building
    enable
    kit
    module
    provide
    testing)
  foreach (_domain IN LISTS _builtin_domains _debug_domains)
    cmake_dependent_option("${PROJECT_NAME}_DEBUG_MODULE_${_domain}" "Enable debugging of ${_domain} logic" OFF
      "${PROJECT_NAME}_DEBUG_MODULE" OFF)
    mark_as_advanced("${PROJECT_NAME}_DEBUG_MODULE_${_domain}")
    if (${PROJECT_NAME}_DEBUG_MODULE_${_domain})
      list(APPEND _vtk_module_log
        "${_domain}")
    endif ()
  endforeach ()
  unset(_domain)
  unset(_builtin_domains)
endif ()
