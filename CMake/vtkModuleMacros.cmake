get_filename_component(_VTKModuleMacros_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

set(_VTKModuleMacros_DEFAULT_LABEL "VTKModular")

include(${_VTKModuleMacros_DIR}/vtkModuleAPI.cmake)
include(GenerateExportHeader)
include(vtkWrapping)
if(VTK_MAKE_INSTANTIATORS)
  include(vtkMakeInstantiator)
endif()
if(UNIX AND VTK_BUILD_FORWARDING_EXECUTABLES)
  include(vtkForwardingExecutable)
endif()

# vtk_module(<name>)
#
# Main function for declaring a VTK module, usually in a module.cmake file in
# the module search path. The module name is the only required argument, all
# others are optional named arguments that will be outlined below. The following
# named options take one (or more) arguments, such as the names of dependent
# modules:
#  DEPENDS = Modules that will be publicly linked to this module
#  PRIVATE_DEPENDS = Modules that will be privately linked to this module
#  COMPILE_DEPENDS = Modules that are needed at compile time by this module
#  TEST_DEPENDS = Modules that are needed by this modules testing executables
#  DESCRIPTION = Free text description of the module
#  TCL_NAME = Alternative name for the TCL wrapping (cannot contain numbers)
#  IMPLEMENTS = Modules that this module implements, using the auto init feature
#  BACKEND = An implementation backend that this module belongs (valid with
#            IMPLEMENTS only)
#  GROUPS = Module groups this module should be included in
#  TEST_LABELS = Add labels to the tests for the module
#
# The following options take no arguments:
#  EXCLUDE_FROM_ALL = Exclude this module from the build all modules flag
#  EXCLUDE_FROM_WRAPPING = Do not attempt to wrap this module in any language
#  EXCLUDE_FROM_WRAP_HIERARCHY = Do not attempt to process with wrap hierarchy
#
# This macro will ensure the module name is compliant, and set the appropriate
# module variables as declared in the module.cmake file.
macro(vtk_module _name)
  vtk_module_check_name(${_name})
  set(vtk-module ${_name})
  set(vtk-module-test ${_name}-Test)
  set(_doing "")
  set(${vtk-module}_DECLARED 1)
  set(${vtk-module-test}_DECLARED 1)
  set(${vtk-module}_DEPENDS "")
  set(${vtk-module}_COMPILE_DEPENDS "")
  set(${vtk-module}_PRIVATE_DEPENDS "")
  set(${vtk-module-test}_DEPENDS "${vtk-module}")
  set(${vtk-module}_IMPLEMENTS "")
  set(${vtk-module}_BACKEND "")
  set(${vtk-module}_DESCRIPTION "description")
  set(${vtk-module}_TCL_NAME "${vtk-module}")
  set(${vtk-module}_EXCLUDE_FROM_ALL 0)
  set(${vtk-module}_EXCLUDE_FROM_WRAPPING 0)
  set(${vtk-module}_EXCLUDE_FROM_WRAP_HIERARCHY 0)
  set(${vtk-module}_TEST_LABELS "")
  set(${vtk-module}_KIT "")
  foreach(arg ${ARGN})
    # XXX: Adding a new keyword? Update Utilities/Maintenance/WhatModulesVTK.py
    # and Utilities/Maintenance/VisualizeModuleDependencies.py as well.
    if("${arg}" MATCHES "^((|COMPILE_|PRIVATE_|TEST_|)DEPENDS|DESCRIPTION|TCL_NAME|IMPLEMENTS|BACKEND|DEFAULT|GROUPS|TEST_LABELS|KIT)$")
      set(_doing "${arg}")
    elseif("${arg}" STREQUAL "EXCLUDE_FROM_ALL")
      set(_doing "")
      set(${vtk-module}_EXCLUDE_FROM_ALL 1)
    elseif("${arg}" STREQUAL "EXCLUDE_FROM_WRAPPING")
      set(_doing "")
      set(${vtk-module}_EXCLUDE_FROM_WRAPPING 1)
    elseif("${arg}" MATCHES "^EXCLUDE_FROM_\([A-Z]+\)_WRAPPING$")
      set(_doing "")
      set(${vtk-module}_EXCLUDE_FROM_${CMAKE_MATCH_1}_WRAPPING 1)
    elseif("${arg}" STREQUAL "EXCLUDE_FROM_WRAP_HIERARCHY")
      set(_doing "")
      set(${vtk-module}_EXCLUDE_FROM_WRAP_HIERARCHY 1)
    elseif("${arg}" MATCHES "^[A-Z][A-Z][A-Z]$" AND
           NOT "${arg}" MATCHES "^(ON|OFF|MPI)$")
      set(_doing "")
      message(AUTHOR_WARNING "Unknown argument [${arg}]")
    elseif("${_doing}" STREQUAL "DEPENDS")
      list(APPEND ${vtk-module}_DEPENDS "${arg}")
    elseif("${_doing}" STREQUAL "TEST_LABELS")
      list(APPEND ${vtk-module}_TEST_LABELS "${arg}")
    elseif("${_doing}" STREQUAL "TEST_DEPENDS")
      list(APPEND ${vtk-module-test}_DEPENDS "${arg}")
    elseif("${_doing}" STREQUAL "COMPILE_DEPENDS")
      list(APPEND ${vtk-module}_COMPILE_DEPENDS "${arg}")
    elseif("${_doing}" STREQUAL "PRIVATE_DEPENDS")
      list(APPEND ${vtk-module}_PRIVATE_DEPENDS "${arg}")
    elseif("${_doing}" STREQUAL "DESCRIPTION")
      set(_doing "")
      set(${vtk-module}_DESCRIPTION "${arg}")
    elseif("${_doing}" STREQUAL "TCL_NAME")
      set(_doing "")
      set(${vtk-module}_TCL_NAME "${arg}")
    elseif("${_doing}" STREQUAL "IMPLEMENTS")
      list(APPEND ${vtk-module}_DEPENDS "${arg}")
      list(APPEND ${vtk-module}_IMPLEMENTS "${arg}")
    elseif("${_doing}" STREQUAL "BACKEND")
      # Backends control groups of implementation modules, a module may be in
      # multiple groups, and it should be an implementation of an interface
      # module. The current BACKENDS are OpenGL and OpenGL2 (new rendering).
      if(NOT DEFINED VTK_BACKEND_${arg}_MODULES)
        list(APPEND VTK_BACKENDS ${arg})
      endif()
      list(APPEND VTK_BACKEND_${arg}_MODULES ${vtk-module})
      list(APPEND ${vtk-module}_BACKEND "${arg}")
      # Being a backend implicitly excludes from all (mutual exclusivity).
      set(${vtk-module}_EXCLUDE_FROM_ALL 1)
    elseif("${_doing}" MATCHES "^DEFAULT")
      message(FATAL_ERROR "Invalid argument [DEFAULT]")
    elseif("${_doing}" STREQUAL "GROUPS")
      # Groups control larger groups of modules.
      if(NOT DEFINED VTK_GROUP_${arg}_MODULES)
        list(APPEND VTK_GROUPS ${arg})
      endif()
      list(APPEND VTK_GROUP_${arg}_MODULES ${vtk-module})
    elseif("${_doing}" STREQUAL "KIT")
      set(${vtk-module}_KIT "${arg}")
    else()
      set(_doing "")
      message(AUTHOR_WARNING "Unknown argument [${arg}]")
    endif()
  endforeach()
  list(SORT ${vtk-module}_DEPENDS) # Deterministic order.
  set(${vtk-module}_LINK_DEPENDS "${${vtk-module}_DEPENDS}")
  list(APPEND ${vtk-module}_DEPENDS
    ${${vtk-module}_COMPILE_DEPENDS}
    ${${vtk-module}_PRIVATE_DEPENDS})
  unset(${vtk-module}_COMPILE_DEPENDS)
  list(SORT ${vtk-module}_DEPENDS) # Deterministic order.
  list(SORT ${vtk-module-test}_DEPENDS) # Deterministic order.
  list(SORT ${vtk-module}_IMPLEMENTS) # Deterministic order.
  if(NOT (${vtk-module}_EXCLUDE_FROM_WRAPPING OR
          ${vtk-module}_EXCLUDE_FROM_TCL_WRAPPING) AND
      "${${vtk-module}_TCL_NAME}" MATCHES "[0-9]")
    message(AUTHOR_WARNING "Specify a TCL_NAME with no digits.")
  endif()
endmacro()

# vtk_module_check_name(<name>)
#
# Check if the proposed module name is compliant.
function(vtk_module_check_name _name)
  if(NOT "${_name}" MATCHES "^[a-zA-Z][a-zA-Z0-9]*$")
    message(FATAL_ERROR "Invalid module name: ${_name}")
  endif()
endfunction()

# vtk_module_impl()
#
# This macro provides module implementation, setting up important variables
# necessary to build a module. It assumes we are in the directory of the module.
macro(vtk_module_impl)
  include(module.cmake OPTIONAL) # Load module meta-data

  vtk_module_config(_dep ${${vtk-module}_DEPENDS})
  if(_dep_INCLUDE_DIRS)
    include_directories(${_dep_INCLUDE_DIRS})
    # This variable is used in vtkWrapping.cmake
    set(${vtk-module}_DEPENDS_INCLUDE_DIRS ${_dep_INCLUDE_DIRS})
  endif()
  if(_dep_LIBRARY_DIRS)
    link_directories(${_dep_LIBRARY_DIRS})
  endif()

  if(NOT DEFINED ${vtk-module}_LIBRARIES)
    foreach(dep IN LISTS ${vtk-module}_LINK_DEPENDS)
      list(APPEND ${vtk-module}_LIBRARIES "${${dep}_LIBRARIES}")
    endforeach()
    if(${vtk-module}_LIBRARIES)
      list(REMOVE_DUPLICATES ${vtk-module}_LIBRARIES)
    endif()
  endif()

  list(APPEND ${vtk-module}_INCLUDE_DIRS
    ${${vtk-module}_BINARY_DIR}
    ${${vtk-module}_SOURCE_DIR})
  list(REMOVE_DUPLICATES ${vtk-module}_INCLUDE_DIRS)

  if(${vtk-module}_INCLUDE_DIRS)
    include_directories(${${vtk-module}_INCLUDE_DIRS})
  endif()
  if(${vtk-module}_SYSTEM_INCLUDE_DIRS)
    include_directories(${${vtk-module}_SYSTEM_INCLUDE_DIRS})
  endif()

  if(${vtk-module}_SYSTEM_LIBRARY_DIRS)
    link_directories(${${vtk-module}_SYSTEM_LIBRARY_DIRS})
  endif()

  if(${vtk-module}_THIRD_PARTY)
    vtk_module_warnings_disable(C CXX)
  endif()
endmacro()

# vtk_module_export_code_find_package(<name>)
#
# Add code that runs when the module is loaded in an application
# to find the given package in the same place VTK found it.
# This is useful for finding external dependencies that provide
# imported targets linked by VTK libraries.
#
# The <name>_DIR variable must be set to the package location.
# The VTK_INSTALL_FIND_PACKAGE_<name>_DIR variable may be set
# to an alternative location for the install tree to reference,
# or to a false value to remove any default location.
macro(vtk_module_export_code_find_package _name)
  if(${_name}_DIR)
    if(DEFINED VTK_INSTALL_FIND_PACKAGE_${_name}_DIR)
      set(_dir "${VTK_INSTALL_FIND_PACKAGE_${_name}_DIR}")
    else()
      set(_dir "${${_name}_DIR}")
    endif()
    if(_dir)
      set(${vtk-module}_EXPORT_CODE_INSTALL "${${vtk-module}_EXPORT_CODE_INSTALL}
if(NOT ${_name}_DIR)
  set(${_name}_DIR \"${_dir}\")
endif()")
    endif()
    set(${vtk-module}_EXPORT_CODE_INSTALL "${${vtk-module}_EXPORT_CODE_INSTALL}
find_package(${_name} REQUIRED QUIET)
")
    set(${vtk-module}_EXPORT_CODE_BUILD "${${vtk-module}_EXPORT_CODE_BUILD}
if(NOT ${_name}_DIR)
  set(${_name}_DIR \"${${_name}_DIR}\")
endif()
find_package(${_name} REQUIRED QUIET)
")
  endif()
endmacro()

# vtk_module_export_info()
#
# Export just the essential data from a module such as name, include directory,
# libraries provided by the module, and any custom variables that are part of
# the module configuration.
macro(vtk_module_export_info)
  vtk_module_impl()
  # First gather and configure the high level module information.
  set(_code "")
  foreach(opt ${${vtk-module}_EXPORT_OPTIONS})
    set(_code "${_code}set(${opt} \"${${opt}}\")\n")
  endforeach()
  if(${vtk-module}_EXCLUDE_FROM_WRAPPING)
    set(_code "${_code}set(${vtk-module}_EXCLUDE_FROM_WRAPPING 1)\n")
  endif()
  if(${vtk-module}_IMPLEMENTS)
    set(_code "${_code}set(${vtk-module}_IMPLEMENTS \"${${vtk-module}_IMPLEMENTS}\")\n")
  endif()
  set(vtk-module-EXPORT_CODE-build "${_code}${${vtk-module}_EXPORT_CODE_BUILD}")
  set(vtk-module-EXPORT_CODE-install "${_code}${${vtk-module}_EXPORT_CODE_INSTALL}")
  if(${vtk-module}_WRAP_HINTS)
    set(vtk-module-EXPORT_CODE-build
      "${vtk-module-EXPORT_CODE-build}set(${vtk-module}_WRAP_HINTS \"${${vtk-module}_WRAP_HINTS}\")\n")
    set(vtk-module-EXPORT_CODE-install
      "${vtk-module-EXPORT_CODE-install}set(${vtk-module}_WRAP_HINTS \"\${CMAKE_CURRENT_LIST_DIR}/${vtk-module}_hints\")\n")
  endif()

  set(vtk-module-DEPENDS "${${vtk-module}_DEPENDS}")
  set(vtk-module-LIBRARIES "${${vtk-module}_LIBRARIES}")
  set(vtk-module-INCLUDE_DIRS-build "${${vtk-module}_INCLUDE_DIRS}")
  set(vtk-module-INCLUDE_DIRS-install "\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_INCLUDE_DIR}")
  if(${vtk-module}_SYSTEM_INCLUDE_DIRS)
    list(APPEND vtk-module-INCLUDE_DIRS-build "${${vtk-module}_SYSTEM_INCLUDE_DIRS}")
    list(APPEND vtk-module-INCLUDE_DIRS-install "${${vtk-module}_SYSTEM_INCLUDE_DIRS}")
  endif()
  if(WIN32)
    set(vtk-module-RUNTIME_LIBRARY_DIRS-build "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set(vtk-module-RUNTIME_LIBRARY_DIRS-install "\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_RUNTIME_DIR}")
  else()
    set(vtk-module-RUNTIME_LIBRARY_DIRS-build "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    set(vtk-module-RUNTIME_LIBRARY_DIRS-install "\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_LIBRARY_DIR}")
  endif()
  set(vtk-module-LIBRARY_DIRS "${${vtk-module}_SYSTEM_LIBRARY_DIRS}")
  set(vtk-module-RUNTIME_LIBRARY_DIRS "${vtk-module-RUNTIME_LIBRARY_DIRS-build}")
  set(vtk-module-INCLUDE_DIRS "${vtk-module-INCLUDE_DIRS-build}")
  set(vtk-module-EXPORT_CODE "${vtk-module-EXPORT_CODE-build}")
  set(vtk-module-WRAP_HIERARCHY_FILE "${${vtk-module}_WRAP_HIERARCHY_FILE}")
  configure_file(${_VTKModuleMacros_DIR}/vtkModuleInfo.cmake.in
    ${VTK_MODULES_DIR}/${vtk-module}.cmake @ONLY)
  set(vtk-module-INCLUDE_DIRS "${vtk-module-INCLUDE_DIRS-install}")
  set(vtk-module-RUNTIME_LIBRARY_DIRS "${vtk-module-RUNTIME_LIBRARY_DIRS-install}")
  set(vtk-module-EXPORT_CODE "${vtk-module-EXPORT_CODE-install}")
  set(vtk-module-WRAP_HIERARCHY_FILE
    "\${CMAKE_CURRENT_LIST_DIR}/${vtk-module}Hierarchy.txt")
  configure_file(${_VTKModuleMacros_DIR}/vtkModuleInfo.cmake.in
    CMakeFiles/${vtk-module}.cmake @ONLY)
  if (NOT VTK_INSTALL_NO_DEVELOPMENT)
    install(FILES ${${vtk-module}_BINARY_DIR}/CMakeFiles/${vtk-module}.cmake
      DESTINATION ${VTK_INSTALL_PACKAGE_DIR}/Modules
      COMPONENT Development)
    if(NOT ${vtk-module}_EXCLUDE_FROM_WRAPPING)
      if(VTK_WRAP_PYTHON OR VTK_WRAP_TCL OR VTK_WRAP_JAVA)
        install(FILES ${${vtk-module}_WRAP_HIERARCHY_FILE}
          DESTINATION ${VTK_INSTALL_PACKAGE_DIR}/Modules
          COMPONENT Development)
      endif()
      if(${vtk-module}_WRAP_HINTS AND EXISTS "${${vtk-module}_WRAP_HINTS}")
        install(FILES ${${vtk-module}_WRAP_HINTS}
          RENAME ${vtk-module}_HINTS
          DESTINATION ${VTK_INSTALL_PACKAGE_DIR}/Modules
          COMPONENT Development)
      endif()
    endif()
  endif()
endmacro()

# vtk_module_export(<sources>)
#
# Export data from a module such as name, include directory and header level
# information useful for wrapping. This calls vtk_module_export_info() and then
# exports additional information in a supplemental file useful for wrapping
# generators.
function(vtk_module_export sources)
  vtk_module_export_info()
  # Now iterate through the headers in the module to get header level information.
  foreach(arg ${sources})
    get_filename_component(src "${arg}" ABSOLUTE)

    string(REGEX REPLACE "\\.(cxx|txx|mm)$" ".h" hdr "${src}")
    if("${hdr}" MATCHES "\\.h$")
      if(EXISTS "${hdr}")
        get_filename_component(_filename "${hdr}" NAME)
        string(REGEX REPLACE "\\.h$" "" _cls "${_filename}")

        get_source_file_property(_wrap_exclude ${src} WRAP_EXCLUDE)
        get_source_file_property(_abstract ${src} ABSTRACT)
        get_source_file_property(_wrap_special ${src} WRAP_SPECIAL)

        if(_wrap_special OR NOT _wrap_exclude)
          list(APPEND vtk-module-HEADERS ${_cls})

          if(_abstract)
            set(vtk-module-ABSTRACT
              "${vtk-module-ABSTRACT}set(${vtk-module}_HEADER_${_cls}_ABSTRACT 1)\n")
          endif()

          if(_wrap_exclude)
            set(vtk-module-WRAP_EXCLUDE
              "${vtk-module-WRAP_EXCLUDE}set(${vtk-module}_HEADER_${_cls}_WRAP_EXCLUDE 1)\n")
          endif()

          if(_wrap_special)
            set(vtk-module-WRAP_SPECIAL
              "${vtk-module-WRAP_SPECIAL}set(${vtk-module}_HEADER_${_cls}_WRAP_SPECIAL 1)\n")
          endif()
        endif()
      endif()
    endif()
  endforeach()
  # Configure wrapping information for external wrapping of headers.
  configure_file(${_VTKModuleMacros_DIR}/vtkModuleHeaders.cmake.in
    ${VTK_MODULES_DIR}/${vtk-module}-Headers.cmake @ONLY)
endfunction()

macro(vtk_module_test)
  if(NOT vtk_module_test_called)
    set(vtk_module_test_called 1) # Run once in a given scope.
    include(../../module.cmake) # Load module meta-data
    vtk_module_config(${vtk-module-test}-Cxx ${${vtk-module-test}-Cxx_DEPENDS})
    if(${vtk-module-test}-Cxx_INCLUDE_DIRS)
      include_directories(${${vtk-module-test}-Cxx_INCLUDE_DIRS})
    endif()
    if(${vtk-module-test}-Cxx_LIBRARY_DIRS)
      link_directories(${${vtk-module-test}-Cxx_LIBRARY_DIRS})
    endif()
  endif()
endmacro()

function(vtk_module_warnings_disable)
  foreach(lang IN LISTS ARGN)
    if(MSVC)
      string(REGEX REPLACE "(^| )[/-]W[0-4]( |$)" " "
        CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w")
    elseif(BORLAND)
      set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w-")
    else()
      set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w")
    endif()
    set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS}" PARENT_SCOPE)
  endforeach()
endfunction()

function(vtk_target_label _target_name)
  if(vtk-module)
    set(_label ${vtk-module})
  else()
    set(_label ${_VTKModuleMacros_DEFAULT_LABEL})
  endif()
  set_property(TARGET ${_target_name} PROPERTY LABELS ${_label})
endfunction()

# vtk_target_name(<name>)
#
# This macro does some basic checking for library naming, and also adds a suffix
# to the output name with the VTK version by default. Setting the variable
# VTK_CUSTOM_LIBRARY_SUFFIX will override the suffix.
function(vtk_target_name _name)
  get_property(_type TARGET ${_name} PROPERTY TYPE)
  if(NOT "${_type}" STREQUAL EXECUTABLE AND NOT VTK_JAVA_INSTALL)
    set_property(TARGET ${_name} PROPERTY VERSION 1)
    set_property(TARGET ${_name} PROPERTY SOVERSION 1)
  endif()
  if("${_name}" MATCHES "^[Vv][Tt][Kk]")
    set(_vtk "")
  else()
    set(_vtk "vtk")
    #message(AUTHOR_WARNING "Target [${_name}] does not start in 'vtk'.")
  endif()
  # Support custom library suffix names, for other projects wanting to inject
  # their own version numbers etc.
  if(DEFINED VTK_CUSTOM_LIBRARY_SUFFIX)
    set(_lib_suffix "${VTK_CUSTOM_LIBRARY_SUFFIX}")
  else()
    set(_lib_suffix "-${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}")
  endif()
  set_property(TARGET ${_name} PROPERTY OUTPUT_NAME ${_vtk}${_name}${_lib_suffix})
endfunction()

function(vtk_target_export _name)
  set_property(GLOBAL APPEND PROPERTY VTK_TARGETS ${_name})
endfunction()

function(vtk_target_install _name)
  if(NOT VTK_INSTALL_NO_LIBRARIES)
    if(APPLE AND VTK_JAVA_INSTALL)
       set_target_properties(${_name} PROPERTIES SUFFIX ".jnilib")
    endif()
    if(VTK_INSTALL_NO_DEVELOPMENT)
      # Installation for deployment does not need static libraries.
      get_property(_type TARGET ${_name} PROPERTY TYPE)
      if(_type STREQUAL "STATIC_LIBRARY")
        return()
      endif()
      set(_archive_destination "")
    else()
      # Installation for development needs static libraries.
      set(_archive_destination
        ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT Development
        )
    endif()
    install(TARGETS ${_name}
      EXPORT ${VTK_INSTALL_EXPORT_NAME}
      RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries
      ${_archive_destination}
      )
  endif()
endfunction()

function(vtk_target _name)
  set(_install 1)
  foreach(arg IN LISTS ARGN)
    if(arg STREQUAL "NO_INSTALL")
      set(_install 0)
    else()
      message(FATAL_ERROR "Unknown argument [${arg}]")
    endif()
  endforeach()
  vtk_target_name(${_name})
  vtk_target_label(${_name})
  vtk_target_export(${_name})
  if(_install)
    vtk_target_install(${_name})
  endif()
endfunction()

#------------------------------------------------------------------------------
# Export a target for a tool that used during the compilation process.
# This is called by vtk_compile_tools_target().
function(vtk_compile_tools_target_export _name)
  set_property(GLOBAL APPEND PROPERTY VTK_COMPILETOOLS_TARGETS ${_name})
endfunction()

#------------------------------------------------------------------------------
function(vtk_compile_tools_target_install _name)
  if(NOT VTK_INSTALL_NO_DEVELOPMENT)
    install(TARGETS ${_name}
      EXPORT ${VTK_INSTALL_EXPORT_NAME}
      RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT Development
      )
  endif()
endfunction()

#------------------------------------------------------------------------------
# vtk_compile_tools_target() is used to declare a target that builds a tool that
# is used during the building process. This macro ensures that the target is
# added to VTK_COMPILETOOLS_TARGETS global property. This also adds install
# rules for the target unless NO_INSTALL argument is specified or
# VTK_INSTALL_NO_DEVELOPMENT variable is set.
function(vtk_compile_tools_target _name)
  if (CMAKE_CROSSCOMPILING)
    message(AUTHOR_WARNING
      "vtk_compile_tools_target is being called when CMAKE_CROSSCOMPILING is true. "
      "This generally signifies a script issue. compile-tools are not expected "
      "to built, but rather imported when CMAKE_CROSSCOMPILING is ON")
  endif ()
  set(_install 1)
  foreach(arg IN LISTS ARGN)
    if(arg STREQUAL "NO_INSTALL")
      set(_install 0)
    else()
      message(FATAL_ERROR "Unknown argument [${arg}]")
    endif()
  endforeach()
  vtk_target_name(${_name})
  vtk_target_label(${_name})
  vtk_compile_tools_target_export(${_name})
  if(_install)
    vtk_compile_tools_target_install(${_name})
  endif()
endfunction()
#------------------------------------------------------------------------------

function(vtk_add_library name)
  add_library(${name} ${ARGN} ${headers})
  if(NOT ARGV1 STREQUAL OBJECT)
    vtk_target(${name})
  endif()
endfunction()

function(vtk_add_executable name)
  if(UNIX AND VTK_BUILD_FORWARDING_EXECUTABLES)
    vtk_add_executable_with_forwarding(VTK_EXE_SUFFIX ${name} ${ARGN})
    set_property(GLOBAL APPEND PROPERTY VTK_TARGETS ${name})
  else()
    add_executable(${name} ${ARGN})
    set_property(GLOBAL APPEND PROPERTY VTK_TARGETS ${name})
  endif()
endfunction()

macro(vtk_module_test_executable test_exe_name)
  vtk_module_test()
  # No forwarding or export for test executables.
  add_executable(${test_exe_name} MACOSX_BUNDLE ${ARGN})
  target_link_libraries(${test_exe_name} LINK_PRIVATE ${${vtk-module-test}-Cxx_LIBRARIES})

  if(${vtk-module-test}-Cxx_DEFINITIONS)
    set_property(TARGET ${test_exe_name} APPEND PROPERTY COMPILE_DEFINITIONS
      ${${vtk-module-test}-Cxx_DEFINITIONS})
  endif()
endmacro()

function(vtk_module_library name)
  if(NOT "${name}" STREQUAL "${vtk-module}")
    message(FATAL_ERROR "vtk_module_library must be invoked with module name")
  endif()

  set(${vtk-module}_LIBRARIES ${vtk-module})
  vtk_module_impl()

  set(vtk-module-HEADERS)
  set(vtk-module-ABSTRACT)
  set(vtk-module-WRAP_SPECIAL)

  # Collect header files matching sources.
  set(_hdrs ${${vtk-module}_HDRS})
  foreach(arg ${ARGN})
    get_filename_component(src "${arg}" ABSOLUTE)

    string(REGEX REPLACE "\\.(cxx|mm)$" ".h" hdr "${src}")
    if("${hdr}" MATCHES "\\.h$" AND EXISTS "${hdr}")
      list(APPEND _hdrs "${hdr}")
    elseif("${src}" MATCHES "\\.txx$" AND EXISTS "${src}")
      list(APPEND _hdrs "${src}")
      string(REGEX REPLACE "\\.txx$" ".h" hdr "${src}")
      if("${hdr}" MATCHES "\\.h$" AND EXISTS "${hdr}")
        list(APPEND _hdrs "${hdr}")
      endif()
    endif()
  endforeach()
  list(APPEND _hdrs "${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}Module.h")
  list(REMOVE_DUPLICATES _hdrs)

  # The instantiators are off by default, and only work on wrapped modules.
  if(VTK_MAKE_INSTANTIATORS AND NOT ${vtk-module}_EXCLUDE_FROM_WRAPPING)
    string(TOUPPER "${vtk-module}_EXPORT" _export_macro)
    vtk_make_instantiator3(${vtk-module}Instantiator _instantiator_SRCS
      "${ARGN}" ${_export_macro} ${CMAKE_CURRENT_BINARY_DIR}
      ${vtk-module}Module.h)
    list(APPEND _hdrs "${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}Instantiator.h")
  endif()

  # Add the vtkWrapHierarchy custom command output to the target, if any.
  # TODO: Re-order things so we do not need to duplicate this condition.
  if(NOT ${vtk-module}_EXCLUDE_FROM_WRAPPING AND
      NOT ${vtk-module}_EXCLUDE_FROM_WRAP_HIERARCHY AND
      ( VTK_WRAP_PYTHON OR VTK_WRAP_TCL OR VTK_WRAP_JAVA ))
    set(_hierarchy ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}Hierarchy.stamp.txt)
  else()
    set(_hierarchy "")
  endif()
  if(CMAKE_GENERATOR MATCHES "Visual Studio 7([^0-9]|$)" AND
      NOT VTK_BUILD_SHARED_LIBS AND _hierarchy)
    # For VS <= 7.1 use explicit dependencies between static libraries
    # to tell CMake to use an ugly workaround for a VS limitation.
    set(_help_vs7 1)
  else()
    set(_help_vs7 0)
  endif()

  set(target_suffix)
  set(force_object)
  set(export_symbol_object)
  if(_vtk_build_as_kit)
    # Hack up the target name to end with 'Objects' and make it an OBJECT
    # library.
    set(target_suffix Objects)
    set(force_object ${target_suffix} OBJECT)
    set(export_symbol_object ${target_suffix} BASE_NAME ${vtk-module})
    # OBJECT libraries don't like this variable being set; clear it.
    unset(${vtk-module}_LIB_DEPENDS CACHE)
  endif()
  vtk_add_library(${vtk-module}${force_object} ${ARGN} ${_hdrs} ${_instantiator_SRCS} ${_hierarchy})
  if(_vtk_build_as_kit)
    # Make an interface library to link with for libraries.
    add_library(${vtk-module} INTERFACE)
    vtk_target_export(${vtk-module})
    vtk_target_install(${vtk-module})
    set_target_properties(${vtk-module}
      PROPERTIES
        INTERFACE_LINK_LIBRARIES "${_vtk_build_as_kit}")
    if(BUILD_SHARED_LIBS)
      # Define a kit-wide export symbol for the objects in this module.
      set_property(TARGET ${vtk-module}Objects APPEND
        PROPERTY
          COMPILE_DEFINITIONS ${${vtk-module}_KIT}_EXPORTS)
      set_target_properties(${vtk-module}Objects
        PROPERTIES
          # Tell generate_export_header what kit-wide export symbol we use.
          DEFINE_SYMBOL ${${vtk-module}_KIT}_EXPORTS
          POSITION_INDEPENDENT_CODE TRUE)
    endif()
  endif()
  foreach(dep IN LISTS ${vtk-module}_LINK_DEPENDS)
    vtk_module_link_libraries(${vtk-module} LINK_PUBLIC ${${dep}_LIBRARIES})
    if(_help_vs7 AND ${dep}_LIBRARIES)
      add_dependencies(${vtk-module} ${${dep}_LIBRARIES})
    endif()
  endforeach()

  # Handle the private dependencies, setting up link/include directories.
  foreach(dep IN LISTS ${vtk-module}_PRIVATE_DEPENDS)
    if(${dep}_INCLUDE_DIRS)
      include_directories(${${dep}_INCLUDE_DIRS})
    endif()
    if(${dep}_LIBRARY_DIRS)
      link_directories(${${dep}_LIBRARY_DIRS})
    endif()
    vtk_module_link_libraries(${vtk-module} LINK_PRIVATE ${${dep}_LIBRARIES})
    if(_help_vs7 AND ${dep}_LIBRARIES)
      add_dependencies(${vtk-module} ${${dep}_LIBRARIES})
    endif()
  endforeach()

  unset(_help_vs7)

  set(sep "")
  if(${vtk-module}_EXPORT_CODE)
    set(sep "\n\n")
  endif()

  # Include module headers from dependencies that need auto-init.
  set(mod_autoinit_deps "")
  foreach(dep IN LISTS ${vtk-module}_LINK_DEPENDS)
    get_property(dep_autoinit GLOBAL PROPERTY ${dep}_NEEDS_AUTOINIT)
    if(dep_autoinit)
      set(mod_autoinit_deps "${mod_autoinit_deps}\n#include \"${dep}Module.h\"")
    endif()
  endforeach()
  if(mod_autoinit_deps)
    set(${vtk-module}_EXPORT_CODE "${${vtk-module}_EXPORT_CODE}${sep}/* AutoInit dependencies.  */${mod_autoinit_deps}")
    set(sep "\n\n")
    set_property(GLOBAL PROPERTY ${vtk-module}_NEEDS_AUTOINIT 1)
  endif()

  # Perform auto-init if this module has or implements an interface.
  if(${vtk-module}_IMPLEMENTED OR ${vtk-module}_IMPLEMENTS)
    set_property(GLOBAL PROPERTY ${vtk-module}_NEEDS_AUTOINIT 1)
    set(${vtk-module}_EXPORT_CODE
      "${${vtk-module}_EXPORT_CODE}${sep}/* AutoInit implementations.  */
#if defined(${vtk-module}_INCLUDE)
# include ${vtk-module}_INCLUDE
#endif
#if defined(${vtk-module}_AUTOINIT)
# include \"vtkAutoInit.h\"
VTK_AUTOINIT(${vtk-module})
#endif")
  endif()

  # Generate the export macro header for symbol visibility/Windows DLL declspec
  if(target_suffix)
    set(${vtk-module}${target_suffix}_EXPORT_CODE
      ${${vtk-module}_EXPORT_CODE})
  endif()
  generate_export_header(${vtk-module}${export_symbol_object} EXPORT_FILE_NAME ${vtk-module}Module.h)
  if (BUILD_SHARED_LIBS)
    # export flags are only added when building shared libs, they cause
    # mismatched visibility warnings when building statically since not all
    # libraries that VTK builds don't set visibility flags. Until we get a
    # time to do that, we skip visibility flags for static libraries.
    add_compiler_export_flags(my_abi_flags)
    set_property(TARGET ${vtk-module}${target_suffix} APPEND
      PROPERTY COMPILE_FLAGS "${my_abi_flags}")
  endif()

  if(BUILD_TESTING AND PYTHON_EXECUTABLE AND NOT ${vtk-module}_NO_HeaderTest AND VTK_SOURCE_DIR)
    string(TOUPPER "${vtk-module}" MOD)
    add_test(NAME ${vtk-module}-HeaderTest
      COMMAND ${PYTHON_EXECUTABLE} ${VTK_SOURCE_DIR}/Testing/Core/HeaderTesting.py
                                   ${CMAKE_CURRENT_SOURCE_DIR} ${MOD}_EXPORT
      )
    set_tests_properties(${vtk-module}-HeaderTest
      PROPERTIES LABELS "${${vtk-module}_TEST_LABELS}"
      )
  endif()

  if(BUILD_TESTING AND TCL_TCLSH)
    add_test(NAME ${vtk-module}-TestSetObjectMacro
      COMMAND ${TCL_TCLSH}
      ${VTK_SOURCE_DIR}/Testing/Core/FindString.tcl
      "${${vtk-module}_SOURCE_DIR}/vtk\\\\*.h"
      # "${CMAKE_CURRENT_SOURCE_DIR}/vtk\\\\*.h"
      "vtkSetObjectMacro"
      ${VTK_SOURCE_DIR}/Common/Core/vtkSetGet.h
      )
    add_test(NAME ${vtk-module}-TestPrintSelf
      COMMAND ${TCL_TCLSH}
      ${VTK_SOURCE_DIR}/Testing/Core/PrintSelfCheck.tcl
      ${${vtk-module}_SOURCE_DIR})
    set_tests_properties(${vtk-module}-TestSetObjectMacro
      PROPERTIES LABELS "${${vtk-module}_TEST_LABELS}"
      )
    set_tests_properties(${vtk-module}-TestPrintSelf
      PROPERTIES LABELS "${${vtk-module}_TEST_LABELS}"
      )
  endif()

  # Add the module to the list of wrapped modules if necessary
  vtk_add_wrapping(${vtk-module} "${ARGN}" "${${vtk-module}_HDRS}")

  # Export the module information.
  vtk_module_export("${ARGN}")

  # Figure out which headers to install.
  if(NOT VTK_INSTALL_NO_DEVELOPMENT AND _hdrs)
    install(FILES ${_hdrs}
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
      COMPONENT Development
      )
  endif()
endfunction()

function(vtk_module_link_libraries module)
  if(VTK_ENABLE_KITS AND ${module}_KIT)
    set_property(GLOBAL APPEND
      PROPERTY
        ${${module}_KIT}_LIBS ${ARGN})
    foreach(dep IN LISTS ARGN)
      if(TARGET ${dep}Objects)
        add_dependencies(${module}Objects ${dep}Objects)
      elseif(TARGET ${dep})
        add_dependencies(${module}Objects ${dep})
      endif()
    endforeach()
  else()
    target_link_libraries(${module} ${ARGN})
  endif()
endfunction()

macro(vtk_module_third_party _pkg)
  string(TOLOWER "${_pkg}" _lower)
  string(TOUPPER "${_pkg}" _upper)

  set(_includes "")
  set(_libs "")
  set(_nolibs 0)
  set(_subdir 1)
  set(_components "")
  set(_optional_components "")
  set(_doing "")
  foreach(arg ${ARGN})
    if("${arg}" MATCHES "^(LIBRARIES|INCLUDE_DIRS|COMPONENTS|OPTIONAL_COMPONENTS)$")
      set(_doing "${arg}")
    elseif("${arg}" STREQUAL "NO_ADD_SUBDIRECTORY")
      set(_doing "")
      set(_subdir 0)
    elseif("${arg}" STREQUAL "NO_LIBRARIES")
      set(_doing "")
      set(_nolibs 1)
    elseif("${_doing}" STREQUAL "LIBRARIES")
      list(APPEND _libs "${arg}")
    elseif("${_doing}" STREQUAL "INCLUDE_DIRS")
      list(APPEND _includes "${arg}")
    elseif("${_doing}" STREQUAL "COMPONENTS")
      list(APPEND _components "${arg}")
    elseif("${_doing}" STREQUAL "OPTIONAL_COMPONENTS")
      list(APPEND _optional_components "${arg}")
    else()
      set(_doing "")
      message(AUTHOR_WARNING "Unknown argument [${arg}]")
    endif()
  endforeach()
  if(_libs AND _nolibs)
    message(FATAL_ERROR "Cannot specify both LIBRARIES and NO_LIBRARIES")
  endif()

  option(VTK_USE_SYSTEM_${_upper} "Use system-installed ${_pkg}" ${VTK_USE_SYSTEM_LIBRARIES})
  mark_as_advanced(VTK_USE_SYSTEM_${_upper})

  if(VTK_USE_SYSTEM_${_upper})
    set(__extra_args)
    if(_components)
      list(APPEND __extra_args ${_components})
    endif()
    if (_optional_components)
      if ("${CMAKE_VERSION}" VERSION_GREATER "2.8.7")
        list(APPEND __extra_args "OPTIONAL_COMPONENTS" ${_optional_components})
      else ()
        # for cmake version <= 2.8.7, since OPTIONAL_COMPONENTS is not
        # available, we just treat them as required components.
        list(APPEND __extra_args ${_optional_components})
      endif()
    endif()
    find_package(${_pkg} REQUIRED ${__extra_args})
    if(NOT ${_upper}_FOUND AND NOT ${_pkg}_FOUND)
      message(FATAL_ERROR "VTK_USE_SYSTEM_${_upper} is ON but ${_pkg} is not found!")
    endif()
    if(${_pkg}_INCLUDE_DIRS)
      set(vtk${_lower}_SYSTEM_INCLUDE_DIRS ${${_pkg}_INCLUDE_DIRS})
    elseif(${_upper}_INCLUDE_DIRS)
      set(vtk${_lower}_SYSTEM_INCLUDE_DIRS ${${_upper}_INCLUDE_DIRS})
    else()
      set(vtk${_lower}_SYSTEM_INCLUDE_DIRS ${${_upper}_INCLUDE_DIR})
    endif()
    if(${_pkg}_LIBRARIES)
      set(vtk${_lower}_LIBRARIES "${${_pkg}_LIBRARIES}")
    else()
      set(vtk${_lower}_LIBRARIES "${${_upper}_LIBRARIES}")
    endif()

    #a workaround for bad FindHDF5 behavior in which deb or opt can
    #end up empty. cmake >= 2.8.12.2 makes this uneccessary
    string(REGEX MATCH "debug;.*optimized;.*"
           _remove_deb_opt "${vtk${_lower}_LIBRARIES}")
    if (_remove_deb_opt)
      set(_tmp ${vtk${_lower}_LIBRARIES})
      list(REMOVE_ITEM _tmp "debug")
      list(REMOVE_ITEM _tmp "optimized")
      list(REMOVE_DUPLICATES _tmp)
      set(vtk${_lower}_LIBRARIES ${_tmp})
    endif()

    set(vtk${_lower}_INCLUDE_DIRS "")
  else()
    if(_nolibs)
      set(vtk${_lower}_LIBRARIES "")
    elseif(_libs)
      set(vtk${_lower}_LIBRARIES "${_libs}")
    else()
      set(vtk${_lower}_LIBRARIES vtk${_lower})
    endif()
    set(vtk${_lower}_INCLUDE_DIRS "${_includes}")
  endif()

  set(vtk${_lower}_THIRD_PARTY 1)
  vtk_module_impl()

  # Export the core module information.
  vtk_module_export_info()

  configure_file(vtk_${_lower}.h.in vtk_${_lower}.h)
  if (NOT VTK_INSTALL_NO_DEVELOPMENT)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vtk_${_lower}.h
            DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
            COMPONENT Development)
  endif()

  if(_subdir AND NOT VTK_USE_SYSTEM_${_upper})
    add_subdirectory(vtk${_lower})
  endif()
endmacro()

# called internally to add one module to the list of available modules
macro(vtk_add_module src f bld ) # [test-langs]
  unset(vtk-module)
  include(${src}/${f} OPTIONAL)
  if(DEFINED vtk-module)
    list(APPEND VTK_MODULES_ALL ${vtk-module})
    get_filename_component(${vtk-module}_BASE ${f} PATH)
    set(${vtk-module}_SOURCE_DIR ${src}/${${vtk-module}_BASE})
    set(${vtk-module}_BINARY_DIR ${bld}/${${vtk-module}_BASE})
    foreach(_lang ${ARGN})
      if(EXISTS ${${vtk-module}_SOURCE_DIR}/Testing/${_lang}/CMakeLists.txt)
        vtk_add_test_module(${_lang})
      endif()
    endforeach()
    if(${vtk-module}_IMPLEMENTS)
      foreach(dep IN LISTS ${vtk-module}_IMPLEMENTS)
        set(${dep}_IMPLEMENTED 1)
      endforeach()
    endif()
  endif()
  unset(vtk-module)
  unset(vtk-module-test)
endmacro()

# called internally to add all the modules undeneath a particular directory to the list of modules
macro(vtk_module_glob src bld) # [test-langs]
  file(GLOB meta RELATIVE "${src}" "${src}/*/*/module.cmake")
  foreach(f ${meta})
    vtk_add_module(${src} ${f} ${bld} ${ARGN})
  endforeach()
endmacro()

# called to search for all modules under the
# vtk_module_src_glob_path and vtk_module_src_path paths to the list
macro(vtk_module_search) # [test-langs]
  set(VTK_MODULES_ALL)

  vtk_module_glob("${VTK_SOURCE_DIR}" "${VTK_BINARY_DIR}" ${ARGN})

  #go through any additional dirs, and make modules of any ./module.cmakes found under them
  foreach(pair ${vtk_module_search_path})
    string(REGEX MATCH "^([^,]*),([^,]*)$" m "${pair}")
    set(src "${CMAKE_MATCH_1}")
    set(bld "${CMAKE_MATCH_2}")
    vtk_add_module("${src}" module.cmake "${bld}" ${ARGN})
  endforeach()

endmacro()

macro(vtk_add_test_module _lang)
  set(_test_module_name ${vtk-module-test}-${_lang})

  list(APPEND VTK_MODULES_ALL ${_test_module_name})
  set(${_test_module_name}_DEPENDS ${${vtk-module-test}_DEPENDS})
  set(${_test_module_name}_SOURCE_DIR ${${vtk-module}_SOURCE_DIR}/Testing/${_lang})
  set(${_test_module_name}_BINARY_DIR ${${vtk-module}_BINARY_DIR}/Testing/${_lang})
  set(${_test_module_name}_IS_TEST 1)
  list(APPEND ${vtk-module}_TESTED_BY ${_test_module_name})
  set(${_test_module_name}_TESTS_FOR ${vtk-module})
  set(${_test_module_name}_DECLARED 1)
  # Exclude test modules from wrapping
  set(${_test_module_name}_EXCLUDE_FROM_WRAPPING 1)
endmacro()
