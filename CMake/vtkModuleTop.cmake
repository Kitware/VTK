#----------------------------------------------------------------------
# Load the module DAG.
set(VTK_MODULES_ALL)
file(GLOB meta RELATIVE "${VTK_SOURCE_DIR}"
   "${VTK_SOURCE_DIR}/*/*/module.cmake" # grouped modules
  )

# Figure out which languages are being wrapped, and add them to the list.
set(_test_languages "Cxx")
if(VTK_WRAP_PYTHON)
  list(APPEND _test_languages "Python")
endif()
if(VTK_WRAP_TCL)
  list(APPEND _test_languages "Tcl")
endif()
if(VTK_WRAP_JAVA)
  list(APPEND _test_languages "Java")
endif()

# Assess modules, and tests in the repository.
foreach(f ${meta})
  unset(vtk-module)
  include(${VTK_SOURCE_DIR}/${f})
  if(DEFINED vtk-module)
    list(APPEND VTK_MODULES_ALL ${vtk-module})
    get_filename_component(${vtk-module}_BASE ${f} PATH)
    set(${vtk-module}_SOURCE_DIR ${VTK_SOURCE_DIR}/${${vtk-module}_BASE})
    set(${vtk-module}_BINARY_DIR ${VTK_BINARY_DIR}/${${vtk-module}_BASE})
    if(BUILD_TESTING)
      # Only add tests for languages that are wrapped.
      foreach(_lang ${_test_languages})
        if(EXISTS ${${vtk-module}_SOURCE_DIR}/Testing/${_lang}/CMakeLists.txt)
          vtk_add_test_module(${_lang})
        endif()
      endforeach()
    endif()
  endif()
endforeach()
# Clear variables set later in each module.
unset(vtk-module)
unset(vtk-module-test)

# Now include the module group logic.
include(vtkGroups)

# Validate the module DAG.
macro(vtk_module_check vtk-module _needed_by stack)
  if(NOT VTK_MODULE_${vtk-module}_DECLARED)
    message(FATAL_ERROR "No such module \"${vtk-module}\" needed by \"${_needed_by}\"")
  endif()
  if(check_started_${vtk-module} AND NOT check_finished_${vtk-module})
    # We reached a module while traversing its own dependencies recursively.
    set(msg "")
    foreach(entry ${stack})
      set(msg " ${entry} =>${msg}")
      if("${entry}" STREQUAL "${vtk-module}")
        break()
      endif()
    endforeach()
    message(FATAL_ERROR "Module dependency cycle detected:\n ${msg} ${vtk-module}")
  elseif(NOT check_started_${vtk-module})
    # Traverse dependencies of this module.  Mark the start and finish.
    set(check_started_${vtk-module} 1)
    foreach(dep IN LISTS VTK_MODULE_${vtk-module}_DEPENDS)
      vtk_module_check(${dep} ${vtk-module} "${vtk-module};${stack}")
    endforeach()
    set(check_finished_${vtk-module} 1)
  endif()
endmacro()

foreach(vtk-module ${VTK_MODULES_ALL})
  vtk_module_check("${vtk-module}" "" "")
endforeach()

#----------------------------------------------------------------------

# Provide an option for all modules.
option(VTK_BUILD_ALL_MODULES "Request to build all modules" OFF)
mark_as_advanced(VTK_BUILD_ALL_MODULES)

# Provide an option for each module.
foreach(vtk-module ${VTK_MODULES_ALL})
  if(NOT ${vtk-module}_IS_TEST)
    option(Module_${vtk-module} "Request building ${vtk-module}"
      ${VTK_MODULE_${vtk-module}_DEFAULT})
    mark_as_advanced(Module_${vtk-module})
    if(VTK_MODULE_${vtk-module}_EXCLUDE_FROM_ALL)
      set(VTK_MODULE_${vtk-module}_IN_ALL 0)
    else()
      set(VTK_MODULE_${vtk-module}_IN_ALL ${VTK_BUILD_ALL_MODULES})
    endif()
  endif()
endforeach()

# Follow dependencies.
macro(vtk_module_enable vtk-module _needed_by)
  if(NOT Module_${vtk-module})
    list(APPEND VTK_MODULE_${vtk-module}_NEEDED_BY ${_needed_by})
  endif()
  if(NOT ${vtk-module}_ENABLED)
    set(${vtk-module}_ENABLED 1)
    foreach(dep IN LISTS VTK_MODULE_${vtk-module}_DEPENDS)
      vtk_module_enable(${dep} ${vtk-module})
    endforeach()

    foreach(test IN LISTS ${vtk-module}_TESTED_BY)
        vtk_module_enable(${test} "")
    endforeach()
  endif()
endmacro()

foreach(vtk-module ${VTK_MODULES_ALL})
  if(Module_${vtk-module} OR VTK_MODULE_${vtk-module}_IN_ALL)
    vtk_module_enable("${vtk-module}" "")
  elseif(VTK_MODULE_${vtk-module}_REQUEST_BY)
    vtk_module_enable("${vtk-module}" "${VTK_MODULE_${vtk-module}_REQUEST_BY}")
  endif()
endforeach()

foreach(vtk-module ${VTK_MODULES_ALL})
  # Exclude modules that exist only to test this module
  # from the report of modules that need this one.  They
  # are enabled exactly because this module is enabled.
  if(VTK_MODULE_${vtk-module}_NEEDED_BY AND ${vtk-module}_TESTED_BY)
    list(REMOVE_ITEM VTK_MODULE_${vtk-module}_NEEDED_BY "${${vtk-module}_TESTED_BY}")
  endif()
endforeach()

# Build final list of enabled modules.
set(VTK_MODULES_ENABLED "")
set(VTK_MODULES_DISABLED "")
foreach(vtk-module ${VTK_MODULES_ALL})
  if(${vtk-module}_ENABLED)
    list(APPEND VTK_MODULES_ENABLED ${vtk-module})
  else()
    list(APPEND VTK_MODULES_DISABLED ${vtk-module})
  endif()
endforeach()
list(SORT VTK_MODULES_ENABLED) # Deterministic order.
list(SORT VTK_MODULES_DISABLED) # Deterministic order.

# Order list to satisfy dependencies.
include(CMake/TopologicalSort.cmake)
topological_sort(VTK_MODULES_ENABLED VTK_MODULE_ _DEPENDS)

# Report what will be built.
foreach(vtk-module ${VTK_MODULES_ENABLED})
  if(NOT ${vtk-module}_IS_TEST)
    if(Module_${vtk-module})
      set(_reason ", requested by Module_${vtk-module}")
    elseif(VTK_MODULE_${vtk-module}_IN_ALL)
      set(_reason ", requested by VTK_BUILD_ALL_MODULES")
    else()
      set(_reason ", needed by [${VTK_MODULE_${vtk-module}_NEEDED_BY}]")
    endif()
    message(STATUS "Enabled ${vtk-module}${_reason}.")
  endif()
endforeach()

# Hide options for modules that will build anyway.
foreach(vtk-module ${VTK_MODULES_ALL})
  if(NOT ${vtk-module}_IS_TEST)
    if(VTK_MODULE_${vtk-module}_IN_ALL OR VTK_MODULE_${vtk-module}_NEEDED_BY)
      set_property(CACHE Module_${vtk-module} PROPERTY TYPE INTERNAL)
    else()
      set_property(CACHE Module_${vtk-module} PROPERTY TYPE BOOL)
    endif()
  endif()
endforeach()

if(NOT VTK_MODULES_ENABLED)
  message(WARNING "No modules enabled!")
  file(REMOVE "${VTK_BINARY_DIR}/VTKTargets.cmake")
  return()
endif()

file(WRITE "${VTK_BINARY_DIR}/VTKTargets.cmake"
  "# Generated by CMake, do not edit!")

macro(verify_vtk_module_is_set)
  if("" STREQUAL "${vtk-module}")
    message(FATAL_ERROR "CMake variable vtk-module is not set")
  endif()
endmacro()

macro(init_module_vars)
  verify_vtk_module_is_set()
  set(${vtk-module}-targets VTKTargets)
  set(${vtk-module}-targets-install "${VTK_INSTALL_PACKAGE_DIR}/VTKTargets.cmake")
  set(${vtk-module}-targets-build "${VTK_BINARY_DIR}/VTKTargets.cmake")
endmacro()

# Build all modules.
foreach(vtk-module ${VTK_MODULES_ENABLED})

  set(_module ${vtk-module})

  if(NOT ${_module}_IS_TEST)
    init_module_vars()
  else()
    set(vtk-module ${${_module}_TESTS_FOR})
  endif()

  include("${${_module}_SOURCE_DIR}/vtk-module-init.cmake" OPTIONAL)
  add_subdirectory("${${_module}_SOURCE_DIR}" "${${_module}_BINARY_DIR}")
endforeach()

#----------------------------------------------------------------------
# Generate VTKConfig* files

# Construct version numbers for VTKConfigVersion.cmake.
SET(_VTK_VERSION_MAJOR ${VTK_MAJOR_VERSION})
SET(_VTK_VERSION_MINOR ${VTK_MINOR_VERSION})
SET(_VTK_VERSION_PATCH ${VTK_BUILD_VERSION})
# We use odd minor numbers for development versions.
# Use a date for the development patch level.
if("${_VTK_VERSION_MINOR}" MATCHES "[13579]$")
  include(${VTK_SOURCE_DIR}/Utilities/KWSys/vtksys/kwsysDateStamp.cmake)
  set(_VTK_VERSION_PATCH
    "${KWSYS_DATE_STAMP_YEAR}${KWSYS_DATE_STAMP_MONTH}${KWSYS_DATE_STAMP_DAY}"
    )
endif()

# Create list of available modules and libraries.
set(VTK_CONFIG_MODULES_ENABLED "")
foreach(vtk-module ${VTK_MODULES_ENABLED})
  if(NOT ${vtk-module}_IS_TEST)
    list(APPEND VTK_CONFIG_MODULES_ENABLED ${vtk-module})
  endif()
endforeach()

# Generate VTKConfig.cmake for the build tree.
set(VTK_CONFIG_CODE "
set(VTK_MODULES_DIR \"${VTK_MODULES_DIR}\")")
set(VTK_CONFIG_CMAKE_DIR "${VTK_SOURCE_DIR}/CMake")
set(VTK_CONFIG_TARGETS_CONDITION " AND NOT VTK_BINARY_DIR")
set(VTK_CONFIG_TARGETS_FILE "${VTK_BINARY_DIR}/VTKTargets.cmake")
set(VTK_CONFIG_MODULE_API_FILE "${VTK_SOURCE_DIR}/CMake/vtkModuleAPI.cmake")
configure_file(CMake/VTKConfig.cmake.in VTKConfig.cmake @ONLY)

# Generate VTKConfig.cmake for the install tree.
set(VTK_CONFIG_CODE "
# Compute the installation prefix from this VTKConfig.cmake file location.
get_filename_component(VTK_INSTALL_PREFIX \"\${CMAKE_CURRENT_LIST_FILE}\" PATH)")
# Construct the proper number of get_filename_component(... PATH)
# calls to compute the installation prefix.
string(REGEX REPLACE "/" ";" _count "${VTK_INSTALL_PACKAGE_DIR}")
foreach(p ${_count})
  set(VTK_CONFIG_CODE "${VTK_CONFIG_CODE}
get_filename_component(VTK_INSTALL_PREFIX \"\${VTK_INSTALL_PREFIX}\" PATH)")
endforeach(p)
set(VTK_CONFIG_CODE "${VTK_CONFIG_CODE}
set(VTK_MODULES_DIR \"\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_PACKAGE_DIR}/Modules\")")
set(VTK_CONFIG_CMAKE_DIR "\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_PACKAGE_DIR}")
set(VTK_CONFIG_TARGETS_CONDITION "")
set(VTK_CONFIG_TARGETS_FILE "\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_PACKAGE_DIR}/VTKTargets.cmake")
set(VTK_CONFIG_MODULE_API_FILE "\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_PACKAGE_DIR}/vtkModuleAPI.cmake")
configure_file(CMake/VTKConfig.cmake.in CMakeFiles/VTKConfig.cmake @ONLY)

configure_file(CMake/VTKConfigVersion.cmake.in VTKConfigVersion.cmake @ONLY)

install(FILES ${VTK_BINARY_DIR}/CMakeFiles/VTKConfig.cmake
              ${VTK_BINARY_DIR}/VTKConfigVersion.cmake
              CMake/vtkModuleAPI.cmake
              CMake/UseVTK.cmake
  DESTINATION ${VTK_INSTALL_PACKAGE_DIR})
get_property(VTK_TARGETS GLOBAL PROPERTY VTK_TARGETS)
if(VTK_TARGETS)
  install(EXPORT VTKTargets DESTINATION ${VTK_INSTALL_PACKAGE_DIR})
else()
  set(CMAKE_CONFIGURABLE_FILE_CONTENT "# No targets!")
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
                 ${VTK_BINARY_DIR}/CMakeFiles/VTKTargets.cmake @ONLY)
  install(FILES ${VTK_BINARY_DIR}/CMakeFiles/VTKTargets.cmake
          DESTINATION ${VTK_INSTALL_PACKAGE_DIR})
endif()
