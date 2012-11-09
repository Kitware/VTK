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

macro(vtk_module _name)
  vtk_module_check_name(${_name})
  set(vtk-module ${_name})
  set(vtk-module-test ${_name}-Test)
  set(_doing "")
  set(${vtk-module}_DECLARED 1)
  set(${vtk-module-test}_DECLARED 1)
  set(${vtk-module}_DEPENDS "")
  set(${vtk-module}_COMPILE_DEPENDS "")
  set(${vtk-module-test}_DEPENDS "${vtk-module}")
  set(${vtk-module}_IMPLEMENTS "")
  set(${vtk-module}_DESCRIPTION "description")
  set(${vtk-module}_TCL_NAME "${vtk-module}")
  set(${vtk-module}_EXCLUDE_FROM_ALL 0)
  set(${vtk-module}_EXCLUDE_FROM_WRAPPING 0)
  set(${vtk-module}_EXCLUDE_FROM_WRAP_HIERARCHY 0)
  set(${vtk-module}_TEST_LABELS "")
  foreach(arg ${ARGN})
  if("${arg}" MATCHES "^((|COMPILE_|TEST_|)DEPENDS|DESCRIPTION|TCL_NAME|IMPLEMENTS|DEFAULT|GROUPS|TEST_LABELS)$")
      set(_doing "${arg}")
    elseif("${arg}" MATCHES "^EXCLUDE_FROM_ALL$")
      set(_doing "")
      set(${vtk-module}_EXCLUDE_FROM_ALL 1)
    elseif("${arg}" MATCHES "^EXCLUDE_FROM_WRAPPING$")
      set(_doing "")
      set(${vtk-module}_EXCLUDE_FROM_WRAPPING 1)
    elseif("${arg}" MATCHES "^EXCLUDE_FROM_WRAP_HIERARCHY$")
      set(_doing "")
      set(${vtk-module}_EXCLUDE_FROM_WRAP_HIERARCHY 1)
    elseif("${arg}" MATCHES "^[A-Z][A-Z][A-Z]$" AND
           NOT "${arg}" MATCHES "^(ON|OFF|MPI)$")
      set(_doing "")
      message(AUTHOR_WARNING "Unknown argument [${arg}]")
    elseif("${_doing}" MATCHES "^DEPENDS$")
      list(APPEND ${vtk-module}_DEPENDS "${arg}")
    elseif("${_doing}" MATCHES "^TEST_LABELS$")
      list(APPEND ${vtk-module}_TEST_LABELS "${arg}")
    elseif("${_doing}" MATCHES "^TEST_DEPENDS$")
      list(APPEND ${vtk-module-test}_DEPENDS "${arg}")
    elseif("${_doing}" MATCHES "^COMPILE_DEPENDS$")
      list(APPEND ${vtk-module}_COMPILE_DEPENDS "${arg}")
    elseif("${_doing}" MATCHES "^DESCRIPTION$")
      set(_doing "")
      set(${vtk-module}_DESCRIPTION "${arg}")
    elseif("${_doing}" MATCHES "^TCL_NAME")
      set(_doing "")
      set(${vtk-module}_TCL_NAME "${arg}")
    elseif("${_doing}" MATCHES "^IMPLEMENTS$")
      list(APPEND ${vtk-module}_DEPENDS "${arg}")
      list(APPEND ${vtk-module}_IMPLEMENTS "${arg}")
    elseif("${_doing}" MATCHES "^DEFAULT")
      message(FATAL_ERROR "Invalid argument [DEFAULT]")
    elseif("${_doing}" MATCHES "^GROUPS")
      # Groups control larger groups of modules.
      if(NOT DEFINED VTK_GROUP_${arg}_MODULES)
        list(APPEND VTK_GROUPS ${arg})
      endif()
      list(APPEND VTK_GROUP_${arg}_MODULES ${vtk-module})
    else()
      set(_doing "")
      message(AUTHOR_WARNING "Unknown argument [${arg}]")
    endif()
  endforeach()
  list(SORT ${vtk-module}_DEPENDS) # Deterministic order.
  set(${vtk-module}_LINK_DEPENDS "${${vtk-module}_DEPENDS}")
  list(APPEND ${vtk-module}_DEPENDS ${${vtk-module}_COMPILE_DEPENDS})
  unset(${vtk-module}_COMPILE_DEPENDS)
  list(SORT ${vtk-module}_DEPENDS) # Deterministic order.
  list(SORT ${vtk-module-test}_DEPENDS) # Deterministic order.
  list(SORT ${vtk-module}_IMPLEMENTS) # Deterministic order.
  if(NOT ${vtk-module}_EXCLUDE_FROM_WRAPPING AND
      "${${vtk-module}_TCL_NAME}" MATCHES "[0-9]")
    message(AUTHOR_WARNING "Specify a TCL_NAME with no digits.")
  endif()
endmacro()

macro(vtk_module_check_name _name)
  if( NOT "${_name}" MATCHES "^[a-zA-Z][a-zA-Z0-9]*$")
    message(FATAL_ERROR "Invalid module name: ${_name}")
  endif()
endmacro()

# This macro provides module implementation, setting up important variables
# necessary to build a module. It assumes we are in the directory of the module.
macro(vtk_module_impl)
  include(module.cmake) # Load module meta-data

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
    set(${vtk-module}_LIBRARIES "")
    foreach(dep IN LISTS ${vtk-module}_LINK_DEPENDS)
      list(APPEND ${vtk-module}_LIBRARIES "${${dep}_LIBRARIES}")
    endforeach()
    if(${vtk-module}_LIBRARIES)
      list(REMOVE_DUPLICATES ${vtk-module}_LIBRARIES)
    endif()
  endif()

  list(APPEND ${vtk-module}_INCLUDE_DIRS
    ${${vtk-module}_BINARY_DIR}
    ${${vtk-module}_SOURCE_DIR}
    )

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

# Export just the essential data from a module such as name, include directory.
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

  set(vtk-module-DEPENDS "${${vtk-module}_DEPENDS}")
  set(vtk-module-LIBRARIES "${${vtk-module}_LIBRARIES}")
  set(vtk-module-INCLUDE_DIRS-build "${${vtk-module}_INCLUDE_DIRS}")
  set(vtk-module-INCLUDE_DIRS-install "\${VTK_INSTALL_PREFIX}/${VTK_INSTALL_INCLUDE_DIR}")
  if(${vtk-module}_SYSTEM_INCLUDE_DIRS)
    list(APPEND vtk-module-INCLUDE_DIRS-build "${${vtk-module}_SYSTEM_INCLUDE_DIRS}")
    list(APPEND vtk-module-INCLUDE_DIRS-install "${${vtk-module}_SYSTEM_INCLUDE_DIRS}")
  endif()
  set(vtk-module-LIBRARY_DIRS "${${vtk-module}_SYSTEM_LIBRARY_DIRS}")
  set(vtk-module-INCLUDE_DIRS "${vtk-module-INCLUDE_DIRS-build}")
  set(vtk-module-EXPORT_CODE "${vtk-module-EXPORT_CODE-build}")
  configure_file(${_VTKModuleMacros_DIR}/vtkModuleInfo.cmake.in
    ${VTK_MODULES_DIR}/${vtk-module}.cmake @ONLY)
  set(vtk-module-INCLUDE_DIRS "${vtk-module-INCLUDE_DIRS-install}")
  set(vtk-module-EXPORT_CODE "${vtk-module-EXPORT_CODE-install}")
  configure_file(${_VTKModuleMacros_DIR}/vtkModuleInfo.cmake.in
    CMakeFiles/${vtk-module}.cmake @ONLY)
  if (NOT VTK_INSTALL_NO_DEVELOPMENT)
    install(FILES ${${vtk-module}_BINARY_DIR}/CMakeFiles/${vtk-module}.cmake
      DESTINATION ${VTK_INSTALL_PACKAGE_DIR}/Modules
      COMPONENT Development)
  endif()
endmacro()

# Export data from a module such as name, include directory and class level
# information useful for wrapping.
function(vtk_module_export sources)
  vtk_module_export_info()
  # Now iterate through the classes in the module to get class level information.
  foreach(arg ${sources})
    get_filename_component(src "${arg}" ABSOLUTE)

    string(REGEX REPLACE "\\.(cxx|mm)$" ".h" hdr "${src}")
    if("${hdr}" MATCHES "\\.h$")
      if(EXISTS "${hdr}")
        get_filename_component(_filename "${hdr}" NAME)
        string(REGEX REPLACE "\\.h$" "" _cls "${_filename}")

        get_source_file_property(_wrap_exclude ${src} WRAP_EXCLUDE)
        get_source_file_property(_abstract ${src} ABSTRACT)
        get_source_file_property(_wrap_special ${src} WRAP_SPECIAL)

        if(_wrap_special OR NOT _wrap_exclude)
          list(APPEND vtk-module-CLASSES ${_cls})

          if(_abstract)
            set(vtk-module-ABSTRACT
              "${vtk-module-ABSTRACT}set(${vtk-module}_CLASS_${_cls}_ABSTRACT 1)\n")
          endif()

          if(_wrap_exclude)
            set(vtk-module-WRAP_EXCLUDE
              "${vtk-module-WRAP_EXCLUDE}set(${vtk-module}_CLASS_${_cls}_WRAP_EXCLUDE 1)\n")
          endif()

          if(_wrap_special)
            set(vtk-module-WRAP_SPECIAL
              "${vtk-module-WRAP_SPECIAL}set(${vtk-module}_CLASS_${_cls}_WRAP_SPECIAL 1)\n")
          endif()
        endif()
      endif()
    endif()
  endforeach()
  # Configure wrapping information for external wrapping of classes.
  configure_file(${_VTKModuleMacros_DIR}/vtkModuleClasses.cmake.in
    ${VTK_MODULES_DIR}/${vtk-module}-Classes.cmake @ONLY)
endfunction()

macro(vtk_module_test)
  if(NOT vtk_module_test_called)
    set(vtk_module_test_called 1) # Run once in a given scope.
    include(../../module.cmake) # Load module meta-data
    vtk_module_config(${vtk-module-test}-Cxx ${${vtk-module-test}-Cxx_DEPENDS})
    if(${vtk-module-test}-Cxx_DEFINITIONS)
      set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
        ${${vtk-module-test}-Cxx_DEFINITIONS})
    endif()
    if(${vtk-module-test}-Cxx_INCLUDE_DIRS)
      include_directories(${${vtk-module-test}-Cxx_INCLUDE_DIRS})
    endif()
    if(${vtk-module-test}-Cxx_LIBRARY_DIRS)
      link_directories(${${vtk-module-test}-Cxx_LIBRARY_DIRS})
    endif()
  endif()
endmacro()

macro(vtk_module_warnings_disable)
  foreach(lang ${ARGN})
    if(MSVC)
      string(REGEX REPLACE "(^| )[/-]W[0-4]( |$)" " "
        CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w")
    elseif(BORLAND)
      set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w-")
    else()
      set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w")
    endif()
  endforeach()
endmacro()

macro(vtk_target_label _target_name)
  if(vtk-module)
    set(_label ${vtk-module})
  else()
    set(_label ${_VTKModuleMacros_DEFAULT_LABEL})
  endif()
  set_property(TARGET ${_target_name} PROPERTY LABELS ${_label})
endmacro()

# This macro does some basic checking for library naming, and also adds a suffix
# to the output name with the VTK version by default. Setting the variable
# VTK_CUSTOM_LIBRARY_SUFFIX will override the suffix.
macro(vtk_target_name _name)
  get_property(_type TARGET ${_name} PROPERTY TYPE)
  if(NOT "${_type}" STREQUAL EXECUTABLE)
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
endmacro()

macro(vtk_target_export _name)
  set_property(GLOBAL APPEND PROPERTY VTK_TARGETS ${_name})
endmacro()

macro(vtk_target_install _name)
  if(NOT VTK_INSTALL_NO_LIBRARIES)
    install(TARGETS ${_name}
      EXPORT ${VTK_INSTALL_EXPORT_NAME}
      RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT Development
      )
  endif()
endmacro()

macro(vtk_target _name)
  set(_install 1)
  foreach(arg ${ARGN})
    if("${arg}" MATCHES "^(NO_INSTALL)$")
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
endmacro()

#------------------------------------------------------------------------------
# Export a target for a tool that used during the compilation process.
# This is called by vtk_compile_tools_target().
macro(vtk_compile_tools_target_export _name)
  set_property(GLOBAL APPEND PROPERTY VTK_COMPILETOOLS_TARGETS ${_name})
endmacro()

#------------------------------------------------------------------------------
macro(vtk_compile_tools_target_install _name)
  if(NOT VTK_INSTALL_NO_DEVELOPMENT)
    install(TARGETS ${_name}
      EXPORT ${VTK_INSTALL_EXPORT_NAME}
      RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT Development
      )
  endif()
endmacro()

#------------------------------------------------------------------------------
# vtk_compile_tools_target() is used to declare a target that builds a tool that
# is used during the building process. This macro ensures that the target is
# added to VTK_COMPILETOOLS_TARGETS global property. This also adds install
# rules for the target unless NO_INSTALL argument is specified or
# VTK_INSTALL_NO_DEVELOPMENT variable is set.
macro(vtk_compile_tools_target _name)
  if (CMAKE_CROSSCOMPILING)
    message(AUTHOR_WARNING
      "vtk_compile_tools_target is being called when CMAKE_CROSSCOMPILING is true. "
      "This generally signifies a script issue. compile-tools are not expected "
      "to built, but rather imported when CMAKE_CROSSCOMPILING is ON")
  endif (CMAKE_CROSSCOMPILING)
 set(_install 1)
  foreach(arg ${ARGN})
    if("${arg}" MATCHES "^(NO_INSTALL)$")
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
endmacro()
#------------------------------------------------------------------------------

function(vtk_add_library name)
  add_library(${name} ${ARGN} ${headers})
  vtk_target(${name})
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
  add_executable(${test_exe_name} ${ARGN})
  target_link_libraries(${test_exe_name} ${${vtk-module-test}-Cxx_LIBRARIES})
endmacro()

function(vtk_module_library name)
  if(NOT "${name}" STREQUAL "${vtk-module}")
    message(FATAL_ERROR "vtk_module_library must be invoked with module name")
  endif()

  set(${vtk-module}_LIBRARIES ${vtk-module})
  vtk_module_impl()

  set(vtk-module-CLASSES)
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
    endif()
  endforeach()
  list(APPEND _hdrs "${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}Module.h")
  list(REMOVE_DUPLICATES _hdrs)

  # Export the module information.
  vtk_module_export("${ARGN}")

  # The instantiators are off by default, and only work on wrapped modules.
  if(VTK_MAKE_INSTANTIATORS AND NOT ${vtk-module}_EXCLUDE_FROM_WRAPPING)
    string(TOUPPER "${vtk-module}_EXPORT" _export_macro)
    vtk_make_instantiator3(${vtk-module}Instantiator _instantiator_SRCS
      "${ARGN}" ${_export_macro} ${CMAKE_CURRENT_BINARY_DIR}
      ${vtk-module}Module.h)
    list(APPEND _hdrs "${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}Instantiator.h")
  endif()

  vtk_add_library(${vtk-module} ${ARGN} ${_hdrs} ${_instantiator_SRCS})
  foreach(dep IN LISTS ${vtk-module}_LINK_DEPENDS)
    target_link_libraries(${vtk-module} ${${dep}_LIBRARIES})
  endforeach()

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
  generate_export_header(${vtk-module} EXPORT_FILE_NAME ${vtk-module}Module.h)
  if (BUILD_SHARED_LIBS)
    # export flags are only added when building shared libs, they cause
    # mismatched visibility warnings when building statically since not all
    # libraries that VTK builds don't set visibility flags. Until we get a
    # time to do that, we skip visibility flags for static builds.
    add_compiler_export_flags(my_abi_flags)
    set_property(TARGET ${vtk-module} APPEND
      PROPERTY COMPILE_FLAGS "${my_abi_flags}")
  endif()

  if(BUILD_TESTING AND PYTHON_EXECUTABLE AND NOT ${vtk-module}_NO_HeaderTest)
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

  # Figure out which headers to install.
  if(NOT VTK_INSTALL_NO_DEVELOPMENT AND _hdrs)
    install(FILES ${_hdrs}
      DESTINATION ${VTK_INSTALL_INCLUDE_DIR}
      COMPONENT Development
      )
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
    elseif("${arg}" MATCHES "^NO_ADD_SUBDIRECTORY$")
      set(_doing "")
      set(_subdir 0)
    elseif("${arg}" MATCHES "^NO_LIBRARIES$")
      set(_doing "")
      set(_nolibs 1)
    elseif("${_doing}" MATCHES "^LIBRARIES$")
      list(APPEND _libs "${arg}")
    elseif("${_doing}" MATCHES "^INCLUDE_DIRS$")
      list(APPEND _includes "${arg}")
    elseif("${_doing}" MATCHES "^COMPONENTS$")
      list(APPEND _components "${arg}")
    elseif("${_doing}" MATCHES "^OPTIONAL_COMPONENTS$")
      list(APPEND _optional_components "${arg}")
    else()
      set(_doing "")
      message(AUTHOR_WARNING "Unknown argument [${arg}]")
    endif()
  endforeach()
  if(_libs AND _nolibs)
    message(FATAL_ERROR "Cannot specify both LIBRARIES and NO_LIBRARIES")
  endif()

  option(VTK_USE_SYSTEM_${_upper} "Use system-installed ${_pkg}" OFF)
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
    if(NOT ${_upper}_FOUND)
      message(FATAL_ERROR "VTK_USE_SYSTEM_${_upper} is ON but ${_pkg} is not found!")
    endif()
    if(${_upper}_INCLUDE_DIRS)
      set(vtk${_lower}_SYSTEM_INCLUDE_DIRS ${${_upper}_INCLUDE_DIRS})
    else()
      set(vtk${_lower}_SYSTEM_INCLUDE_DIRS ${${_upper}_INCLUDE_DIR})
    endif()
    set(vtk${_lower}_LIBRARIES "${${_upper}_LIBRARIES}")
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
