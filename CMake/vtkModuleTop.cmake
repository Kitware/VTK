# Figure out which languages are being wrapped, and add them to the list.
if(BUILD_TESTING)
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
else()
  set(_test_languages "")
endif()

#----------------------------------------------------------------------
# Load the module DAG.

# Remove old ${vtk-module}.*.cmake files generated for enabled modules.
# These will be re-written on currently enabled modules.
file(GLOB module_files "${VTK_MODULES_DIR}/*.cmake")
if (module_files)
  file(REMOVE ${module_files})
endif()

# This is a little hackish, but define the rendering backend if none was passed
# in for the first CMake invocation for modules that depend on the backend
# chosen.
if(NOT DEFINED VTK_RENDERING_BACKEND)
  # has the application defined a desired default for the backend?
  # if not, use VTKs default of OpenGL2
  if(DEFINED VTK_RENDERING_BACKEND_DEFAULT)
    set(VTK_RENDERING_BACKEND ${VTK_RENDERING_BACKEND_DEFAULT})
  else()
    set(VTK_RENDERING_BACKEND "OpenGL2")
  endif()
  set(_backend_set_for_first_cmake TRUE)
endif()

# Assess modules, and tests in the repository.
vtk_module_search(${_test_languages})

# Need to ensure the state is restored so that cache variable defaults can be
# added if necessary on the first CMake run, offering up the choice of backend.
if(_backend_set_for_first_cmake)
  unset(VTK_RENDERING_BACKEND)
endif()

# Now include the module group logic.
include(vtkGroups)

# Now figure out which rendering backend to use.
include(vtkBackends)

# Validate the module DAG.
macro(vtk_module_check vtk-module _needed_by stack)
  if(NOT ${vtk-module}_DECLARED)
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
    foreach(dep IN LISTS ${vtk-module}_DEPENDS)
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

include(CMakeDependentOption)
cmake_dependent_option(VTK_BUILD_ALL_MODULES_FOR_TESTS
  "Enable modules as needed for testing all the enabled modules" OFF
  "BUILD_TESTING" OFF)
mark_as_advanced(VTK_BUILD_ALL_MODULES_FOR_TESTS)

# Provide an option for each module.
foreach(vtk-module ${VTK_MODULES_ALL})
  if(NOT ${vtk-module}_IS_TEST)
    option(Module_${vtk-module} "Request building ${vtk-module}"
      ${${vtk-module}_DEFAULT})
    mark_as_advanced(Module_${vtk-module})
    if(${vtk-module}_EXCLUDE_FROM_ALL)
      set(${vtk-module}_IN_ALL 0)
    else()
      set(${vtk-module}_IN_ALL ${VTK_BUILD_ALL_MODULES})
    endif()
  endif()
endforeach()

# Follow dependencies.
macro(vtk_module_enable vtk-module _needed_by)
  if(NOT Module_${vtk-module})
    list(APPEND ${vtk-module}_NEEDED_BY ${_needed_by})
  endif()
  if(NOT ${vtk-module}_ENABLED)
    set(${vtk-module}_ENABLED 1)
    foreach(dep IN LISTS ${vtk-module}_DEPENDS)
      vtk_module_enable(${dep} ${vtk-module})
    endforeach()
    foreach(imp IN LISTS ${vtk-module}_IMPLEMENTATIONS)
      vtk_module_enable(${imp} ${vtk-module})
    endforeach()

    # If VTK_BUILD_ALL_MODULES_FOR_TESTS is true, then and then
    # alone do we include the test modules in building build the dependency
    # graph for enabled modules (BUG #13297).
    if (VTK_BUILD_ALL_MODULES_FOR_TESTS)
      foreach(test IN LISTS ${vtk-module}_TESTED_BY)
        vtk_module_enable(${test} "")
      endforeach()
    elseif (BUILD_TESTING)
      # identify vtkTesting<> dependencies on the test module and enable them.
      # this ensures that core testing modules such as vtkTestingCore,
      # vtkTestingRendering which many test modules depend on, are automatically
      # enabled.
      foreach(test IN LISTS ${vtk-module}_TESTED_BY)
        foreach(test-depends IN LISTS ${test}_DEPENDS)
          if (test-depends MATCHES "^vtkTesting.*")
            vtk_module_enable(${test-depends} "")
          endif()
        endforeach()
      endforeach()
    endif()
  endif()
endmacro()

foreach(vtk-module ${VTK_MODULES_ALL})
  if(Module_${vtk-module} OR ${vtk-module}_IN_ALL)
    vtk_module_enable("${vtk-module}" "")
  elseif(${vtk-module}_REQUEST_BY)
    vtk_module_enable("${vtk-module}" "${${vtk-module}_REQUEST_BY}")
  endif()
endforeach()

foreach(vtk-module ${VTK_MODULES_ALL})
  # Exclude modules that exist only to test this module
  # from the report of modules that need this one.  They
  # are enabled exactly because this module is enabled.
  if(${vtk-module}_NEEDED_BY AND ${vtk-module}_TESTED_BY)
    list(REMOVE_ITEM ${vtk-module}_NEEDED_BY "${${vtk-module}_TESTED_BY}")
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

if (NOT VTK_BUILD_ALL_MODULES_FOR_TESTS)
  # If VTK_BUILD_ALL_MODULES_FOR_TESTS is OFF, it implies that we didn't add any
  # test modules to the dependecy graph. We now add the test modules for all
  # enabled modules iff the all the test dependecies are already satisfied
  # (BUG #13297).
  foreach(vtk-module IN LISTS VTK_MODULES_ENABLED)
    foreach(test IN LISTS ${vtk-module}_TESTED_BY)
      # if all test-dependencies are satisfied, enable it.
      set (missing_dependencis)
      foreach(test-depends IN LISTS ${test}_DEPENDS)
        list(FIND VTK_MODULES_ENABLED ${test-depends} found)
        if (found EQUAL -1)
          list(APPEND missing_dependencis ${test-depends})
        endif()
      endforeach()
      if (NOT missing_dependencis)
        vtk_module_enable(${test} "")
        list(APPEND VTK_MODULES_ENABLED ${test})
      else()
        message(STATUS
        "Disable test module ${test} since required modules are not enabled: ${missing_dependencis}")
      endif()
    endforeach()
  endforeach()
endif()

list(SORT VTK_MODULES_ENABLED) # Deterministic order.
list(SORT VTK_MODULES_DISABLED) # Deterministic order.

# Order list to satisfy dependencies.
include(CMake/TopologicalSort.cmake)
topological_sort(VTK_MODULES_ENABLED "" _DEPENDS)
set(vtk_modules_and_kits ${VTK_MODULES_ENABLED})
if(VTK_ENABLE_KITS)
  set(vtk_kits)
  foreach(module IN LISTS VTK_MODULES_ENABLED)
    if(${module}_KIT)
      set(kit ${${module}_KIT})
      # Set kit management variables.
      set(_${kit}_is_kit 1)
      list(APPEND _${kit}_modules ${module})
      list(APPEND vtk_kits ${kit})
      list(APPEND ${kit}_KIT_DEPENDS ${module})
    else()
      set(kit ${module})
    endif()

    # The module graph is modified so that any M1 -> M2 dependency, if ${M2_KIT}
    # is set, an edge is added for M1 -> ${M2_KIT}, however, if ${M1_KIT} is the
    # same as ${M2_KIT}, the kit dependency is ignored.

    foreach(dep IN LISTS ${module}_DEPENDS)
      if(${dep}_KIT)
        # Ignore self dependencies.
        if(NOT kit STREQUAL ${dep}_KIT)
          # Depend on the dependency's kit.
          list(APPEND ${module}_KIT_DEPENDS ${${dep}_KIT})
        endif()
      endif()

      # Keep the original dependency.
      list(APPEND ${module}_KIT_DEPENDS ${dep})
    endforeach()
  endforeach()

  list(REMOVE_DUPLICATES vtk_kits)

  # Put all kits in the list (if they are not dependencies of any module, they
  # will be dropped otherwise).
  list(APPEND vtk_modules_and_kits ${vtk_kits})

  # Sort all modules and kits.
  topological_sort(vtk_modules_and_kits "" _KIT_DEPENDS)
endif()


# Report what will be built.
set(_modules_enabled_alpha "${VTK_MODULES_ENABLED}")
list(SORT _modules_enabled_alpha)
list(REMOVE_ITEM _modules_enabled_alpha vtkWrappingJava vtkWrappingPythonCore)
list(LENGTH _modules_enabled_alpha _length)
message(STATUS "Enabled ${_length} modules:")
foreach(vtk-module ${_modules_enabled_alpha})
  if(NOT ${vtk-module}_IS_TEST)
    if(Module_${vtk-module})
      set(_reason ", requested by Module_${vtk-module}.")
    elseif(${vtk-module}_IN_ALL)
      set(_reason ", requested by VTK_BUILD_ALL_MODULES.")
    else()
      set(_needed_by "${${vtk-module}_NEEDED_BY}")
      list(SORT _needed_by)
      list(LENGTH _needed_by _length)
      if(_length GREATER "1")
        set(_reason ", needed by ${_length} modules:")
        foreach(dep ${_needed_by})
          set(_reason "${_reason}\n        ${dep}")
        endforeach()
      else()
        set(_reason ", needed by ${_needed_by}.")
      endif()
    endif()
    if(VTK_ENABLE_KITS AND ${vtk-module}_KIT)
      set(_kit " (kit: ${${vtk-module}_KIT})")
    else()
      set(_kit)
    endif()
    message(STATUS " * ${vtk-module}${_kit}${_reason}")
  endif()
endforeach()

# Hide options for modules that will build anyway.
foreach(vtk-module ${VTK_MODULES_ALL})
  if(NOT ${vtk-module}_IS_TEST)
    if(${vtk-module}_IN_ALL OR ${vtk-module}_NEEDED_BY)
      set_property(CACHE Module_${vtk-module} PROPERTY TYPE INTERNAL)
    else()
      set_property(CACHE Module_${vtk-module} PROPERTY TYPE BOOL)
    endif()
  endif()
endforeach()

#hide options of modules that are part of a different backend
# or are required by the backend
foreach(backend ${VTK_BACKENDS})
  foreach(module ${VTK_BACKEND_${backend}_MODULES})
    if(NOT ${module}_IS_TEST)
      if((NOT (${backend} STREQUAL "${VTK_RENDERING_BACKEND}")) OR
        ${module}_IMPLEMENTATION_REQUIRED_BY_BACKEND)
        set_property(CACHE Module_${module} PROPERTY TYPE INTERNAL)
      endif()
    endif()
  endforeach()
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

# VTK_WRAP_PYTHON_MODULES can be used to explicitly control which modules
# get Python wrapped (no automatic dependency support is provided at this
# time). If it has been set mark the modules in the list as such.
# Note any wrap exclude entries in the module.cmake will take precedence
# If entry has not been set default to VTK_MODULES_ENABLED.
if(VTK_WRAP_PYTHON)
  if(NOT VTK_WRAP_PYTHON_MODULES)
    set(VTK_WRAP_PYTHON_MODULES ${VTK_MODULES_ENABLED})
  endif()
endif()

macro(_vtk_build_module _module)
  set(vtk-module ${_module})

  if(NOT ${_module}_IS_TEST)
    init_module_vars()
  else()
    set(vtk-module ${${_module}_TESTS_FOR})
  endif()

  include("${${_module}_SOURCE_DIR}/vtk-module-init.cmake" OPTIONAL)
  add_subdirectory("${${_module}_SOURCE_DIR}" "${${_module}_BINARY_DIR}")
endmacro()

# Build all modules.
foreach(kit IN LISTS vtk_modules_and_kits)
  if(_${kit}_is_kit)
    set(_vtk_build_as_kit ${kit})
    set(kit_srcs)
    foreach(kit_module IN LISTS _${kit}_modules)
      list(APPEND kit_srcs $<TARGET_OBJECTS:${kit_module}Objects>)
    endforeach()

    configure_file("${_VTKModuleMacros_DIR}/vtkKit.cxx.in"
      "${CMAKE_CURRENT_BINARY_DIR}/${kit}Kit.cxx" @ONLY)
    add_library(${kit} "${CMAKE_CURRENT_BINARY_DIR}/${kit}Kit.cxx" ${kit_srcs})
    get_property(kit_libs GLOBAL
      PROPERTY
        ${kit}_LIBS)
    set(kit_priv)
    set(kit_pub)
    set(is_priv)
    foreach(lib IN LISTS kit_libs)
      if(lib STREQUAL LINK_PUBLIC)
        set(is_priv 0)
      elseif(lib STREQUAL LINK_PRIVATE)
        set(is_priv 1)
      else()
        if(${lib}_KIT)
          set(lib ${${lib}_KIT})
        endif()
        if(is_priv)
          list(APPEND kit_priv ${lib})
        else()
          list(APPEND kit_pub ${lib})
        endif()
      endif()
    endforeach()
    if(kit_priv)
      list(REMOVE_DUPLICATES kit_priv)
      list(REMOVE_ITEM kit_priv ${kit})
    endif()
    if(kit_pub)
      list(REMOVE_DUPLICATES kit_pub)
      list(REMOVE_ITEM kit_pub ${kit})
    endif()
    target_link_libraries(${kit}
      LINK_PRIVATE ${kit_priv}
      LINK_PUBLIC  ${kit_pub})
    vtk_target(${kit})
  else()
    if(VTK_ENABLE_KITS)
      set(_vtk_build_as_kit ${${kit}_KIT})
    else()
      set(_vtk_build_as_kit)
    endif()
    _vtk_build_module(${kit})
  endif()
endforeach()
unset(vtk-module)

#----------------------------------------------------------------------
# Generate VTKConfig* files

# Construct version numbers for VTKConfigVersion.cmake.
SET(_VTK_VERSION_MAJOR ${VTK_MAJOR_VERSION})
SET(_VTK_VERSION_MINOR ${VTK_MINOR_VERSION})
SET(_VTK_VERSION_PATCH ${VTK_BUILD_VERSION})

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
# Target used to ensure VTKConfig is load just once
set(VTK_COMMON_TARGET vtkCommonCore)
configure_file(CMake/VTKConfig.cmake.in VTKConfig.cmake @ONLY)

# Generate VTKConfig.cmake for the install tree.
set(VTK_CONFIG_CODE "
# Compute the installation prefix from this VTKConfig.cmake file location.
set(_vtk_installed_prefix \"${CMAKE_INSTALL_PREFIX}/${VTK_INSTALL_PACKAGE_DIR}\")
set(_vtk_requested_prefix \"\${CMAKE_CURRENT_LIST_DIR}\")
get_filename_component(_vtk_installed_prefix_full \"\${_vtk_installed_prefix}\" REALPATH)
get_filename_component(_vtk_requested_prefix_full \"\${_vtk_requested_prefix}\" REALPATH)
if (_vtk_installed_prefix_full STREQUAL _vtk_requested_prefix_full)
  set(VTK_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")
else ()
  set(VTK_INSTALL_PREFIX \"\${CMAKE_CURRENT_LIST_DIR}\")")

# Construct the proper number of get_filename_component(... PATH)
# calls to compute the installation prefix.
string(REGEX REPLACE "/" ";" _count "${VTK_INSTALL_PACKAGE_DIR}")
foreach(p ${_count})
  set(VTK_CONFIG_CODE "${VTK_CONFIG_CODE}
  get_filename_component(VTK_INSTALL_PREFIX \"\${VTK_INSTALL_PREFIX}\" PATH)")
endforeach()

set(VTK_CONFIG_CODE "${VTK_CONFIG_CODE}
endif ()")

set(VTK_CONFIG_CODE "${VTK_CONFIG_CODE}
set(VTK_MODULES_DIR \"\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_PACKAGE_DIR}/Modules\")")
set(VTK_CONFIG_CMAKE_DIR "\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_PACKAGE_DIR}")
set(VTK_CONFIG_TARGETS_CONDITION "")
set(VTK_CONFIG_TARGETS_FILE "\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_PACKAGE_DIR}/VTKTargets.cmake")
set(VTK_CONFIG_MODULE_API_FILE "\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_PACKAGE_DIR}/vtkModuleAPI.cmake")
configure_file(CMake/VTKConfig.cmake.in CMakeFiles/VTKConfig.cmake @ONLY)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${VTK_BINARY_DIR}/VTKConfigVersion.cmake
  VERSION ${_VTK_VERSION_MAJOR}.${_VTK_VERSION_MINOR}.${_VTK_VERSION_PATCH}
  COMPATIBILITY SameMajorVersion
  )

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES ${VTK_BINARY_DIR}/CMakeFiles/VTKConfig.cmake
                ${VTK_BINARY_DIR}/VTKConfigVersion.cmake
                CMake/vtkexportheader.cmake.in
                CMake/VTKGenerateExportHeader.cmake
                CMake/pythonmodules.h.in
                CMake/UseVTK.cmake
                CMake/FindTCL.cmake
                CMake/TopologicalSort.cmake
                CMake/vtkTclTkMacros.cmake
                CMake/vtk-forward.c.in
                CMake/vtkGroups.cmake
                CMake/vtkForwardingExecutable.cmake
                CMake/vtkJavaWrapping.cmake
                CMake/vtkMakeInstantiator.cmake
                CMake/vtkMakeInstantiator.cxx.in
                CMake/vtkMakeInstantiator.h.in
                CMake/vtkModuleAPI.cmake
                CMake/vtkModuleHeaders.cmake.in
                CMake/vtkModuleInfo.cmake.in
                CMake/vtkModuleMacros.cmake
                CMake/vtkModuleMacrosPython.cmake
                CMake/vtkMPI.cmake
                CMake/vtkExternalModuleMacros.cmake
                CMake/vtkObjectFactory.cxx.in
                CMake/vtkObjectFactory.h.in
                CMake/vtkPythonPackages.cmake
                CMake/vtkPythonWrapping.cmake
                CMake/vtkTclWrapping.cmake
                CMake/vtkThirdParty.cmake
                CMake/vtkWrapHierarchy.cmake
                CMake/vtkWrapJava.cmake
                CMake/vtkWrapperInit.data.in
                CMake/vtkWrapping.cmake
                CMake/vtkWrapPython.cmake
                CMake/vtkWrapPythonSIP.cmake
                CMake/vtkWrapPython.sip.in
                CMake/vtkWrapTcl.cmake

    DESTINATION ${VTK_INSTALL_PACKAGE_DIR})
  get_property(VTK_TARGETS GLOBAL PROPERTY VTK_TARGETS)
  if(VTK_TARGETS)
    install(EXPORT ${VTK_INSTALL_EXPORT_NAME}  DESTINATION ${VTK_INSTALL_PACKAGE_DIR} FILE ${VTK_INSTALL_EXPORT_NAME}.cmake)
  else()
    set(CMAKE_CONFIGURABLE_FILE_CONTENT "# No targets!")
    configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
                   ${VTK_BINARY_DIR}/CMakeFiles/VTKTargets.cmake @ONLY)
    install(FILES ${VTK_BINARY_DIR}/CMakeFiles/VTKTargets.cmake
            DESTINATION ${VTK_INSTALL_PACKAGE_DIR})
  endif()
endif()
