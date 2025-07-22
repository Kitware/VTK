#[==[.rst:
***********************
vtkModuleWrapJavaScript
***********************
#]==]


#[==[.rst:
APIs for wrapping modules for JavaScript
#]==]

#[==[.rst:
..  cmake:command:: _vtk_module_wrap_javascript_sources


  Generate sources for using a module's classes from JavaScript. |module-impl|

  This function generates the wrapped sources for a module. It places the list of
  generated source files and classes in variables named in the second and third
  arguments, respectively.

  .. code-block:: cmake

    _vtk_module_wrap_javascript_sources(<module> <sources> <classes>)

#]==]
function (_vtk_module_wrap_javascript_sources module sources classes)
  # the library name for a VTK::ModuleName is vtkModuleName
  _vtk_module_get_module_property("${_vtk_javascript_module}"
    PROPERTY  "library_name"
    VARIABLE  _vtk_library_name)
  # The real target name for VTK::ModuleName is ModuleName
  _vtk_module_real_target(_vtk_javascript_target_name "${_vtk_javascript_module}")
  # The library name will be vtkModuleName.js
  set(_vtk_javascript_library_name "${_vtk_library_name}.js")

  file (MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_javascript_library_name}")
  set(_vtk_javascript_args_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_javascript_library_name}/${_vtk_javascript_library_name}.$<CONFIGURATION>.args")

  set(_vtk_javascript_hierarchy_depends "${module}")
  # Get private dependencies of `module`
  _vtk_module_get_module_property("${module}"
    PROPERTY  "private_depends"
    VARIABLE  _vtk_javascript_private_depends)
  list(APPEND _vtk_javascript_hierarchy_depends
    ${_vtk_javascript_private_depends})
  # Get optional dependencies of `module`
  _vtk_module_get_module_property("${module}"
    PROPERTY  "optional_depends"
    VARIABLE  _vtk_javascript_optional_depends)
  # Appended only if those optional dependencies are declared as targets
  foreach (_vtk_javascript_optional_depend IN LISTS _vtk_javascript_optional_depends)
    if (TARGET "${_vtk_javascript_optional_depend}")
      list(APPEND _vtk_javascript_hierarchy_depends
        "${_vtk_javascript_optional_depend}")
    endif ()
  endforeach ()

  set(_vtk_javascript_command_depends)
  set(_vtk_javascript_hierarchy_files)
  foreach (_vtk_javascript_hierarchy_depend IN LISTS _vtk_javascript_hierarchy_depends)
    _vtk_module_get_module_property("${_vtk_javascript_hierarchy_depend}"
      PROPERTY "hierarchy"
      VARIABLE _vtk_javascript_hierarchy_file)
    if (_vtk_javascript_hierarchy_file)
      list(APPEND _vtk_javascript_hierarchy_files "${_vtk_javascript_hierarchy_file}")
      get_property(_vtk_javascript_is_imported
        TARGET   "${_vtk_javascript_hierarchy_depend}"
        PROPERTY "IMPORTED")
      if (_vtk_javascript_is_imported OR CMAKE_GENERATOR MATCHES "Ninja")
        list(APPEND _vtk_javascript_command_depends "${_vtk_javascript_hierarchy_file}")
      else ()
        _vtk_module_get_module_property("${_vtk_javascript_hierarchy_depend}"
          PROPERTY "library_name"
          VARIABLE _vtk_javascript_hierarchy_library_name)
        if (TARGET "${_vtk_javascript_hierarchy_library_name}-hierarchy")
          list(APPEND _vtk_javascript_command_depends "${_vtk_javascript_hierarchy_library_name}-hierarchy")
        else ()
          message(FATAL_ERROR
            "The ${_vtk_javascript_hierarchy_depend} hierarchy file is attached to a non-imported target "
            "and a hierarchy target (${_vtk_javascript_hierarchy_library_name}-hierarchy) is "
            "missing.")
        endif ()
      endif ()
    endif ()
  endforeach ()

  set(_vtk_javascript_genex_allowed 1)
  if (CMAKE_VERSION VERSION_LESS "3.19")
    get_property(_vtk_javascript_target_type
      TARGET  "${_vtk_javascript_target_name}"
      PROPERTY TYPE)
    if (_vtk_javascript_target_type STREQUAL "INTERFACE_LIBRARY")
      set(_vtk_javascript_genex_allowed 0)
    endif ()
  endif ()

  set(_vtk_javascript_genex_compile_definitions "")
  set(_vtk_javascript_genex_include_directories "")
  if (_vtk_javascript_genex_allowed)
    set(_vtk_javascript_genex_compile_definitions
      "$<TARGET_PROPERTY:${_vtk_javascript_target_name},COMPILE_DEFINITIONS>")
    set(_vtk_javascript_genex_include_directories
      "$<TARGET_PROPERTY:${_vtk_javascript_target_name},INCLUDE_DIRECTORIES>")
  else ()
    if (NOT DEFINED ENV{CI})
      message(AUTHOR_WARNING
        "JavaScript wrapping is not using target-local compile definitions or "
        "include directories. This may affect generation of the JavaScript "
        "wrapper sources for the ${module} module. Use CMake 3.19+ to "
        "guarantee intended behavior.")
    endif ()
  endif ()
  file(GENERATE
    OUTPUT  "${_vtk_javascript_args_file}"
    CONTENT "$<$<BOOL:${_vtk_javascript_genex_compile_definitions}>:\n-D\'$<JOIN:${_vtk_javascript_genex_compile_definitions},\'\n-D\'>\'>\n
$<$<BOOL:${_vtk_javascript_genex_include_directories}>:\n-I\'$<JOIN:${_vtk_javascript_genex_include_directories},\'\n-I\'>\'>\n
$<$<BOOL:${_vtk_javascript_hierarchy_files}>:\n--types \'$<JOIN:${_vtk_javascript_hierarchy_files},\'\n--types \'>\'>\n")

  # Get the list of public headers from the module
  _vtk_module_get_module_property("${module}"
    PROPERTY  "headers"
    VARIABLE  _vtk_javascript_headers)
  set(_vtk_javascript_library_classes)
  set(_vtk_javascript_library_sources)
  include(vtkModuleWrapJavaScriptExclusions OPTIONAL)

  foreach (_vtk_javascript_header IN LISTS _vtk_javascript_headers)
    # Assume the class name matches the basename of the header file. This is a VTK convention
    get_filename_component(_vtk_javascript_basename "${_vtk_javascript_header}" NAME_WE)
    list(APPEND _vtk_javascript_library_classes
      "${_vtk_javascript_basename}")
    set(_vtk_javascript_source_output
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_javascript_library_name}/${_vtk_javascript_basename}Embinding.cxx")
    set(_vtk_javascript_depfile_genex
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_javascript_library_name}/${_vtk_javascript_basename}Embinding.cxx.$<CONFIG>.d")
    set(_vtk_javascript_depfile_nogenex
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_javascript_library_name}/${_vtk_javascript_basename}Embinding.cxx.d")  
    list(APPEND _vtk_javascript_library_sources
      "${_vtk_javascript_source_output}")

    set(_vtk_javascript_wrap_target "VTK::WrapJavaScript")
    set(_vtk_javascript_macros_args)
    if (TARGET VTKCompileTools::WrapJavaScript)
      set(_vtk_javascript_wrap_target "VTKCompileTools::WrapJavaScript")
      if (TARGET VTKCompileTools_macros)
        list(APPEND _vtk_javascript_command_depends
          "VTKCompileTools_macros")
        list(APPEND _vtk_javascript_macros_args
          -undef
          -imacros "${_VTKCompileTools_macros_file}")
      endif ()
    endif ()

    _vtk_module_depfile_args(
      MULTI_CONFIG_NEEDS_GENEX
      TOOL_ARGS _vtk_javascript_depfile_flags
      CUSTOM_COMMAND_ARGS _vtk_javascript_depfile_args
      SOURCE "${_vtk_javascript_header}"
      DEPFILE_PATH "${_vtk_javascript_depfile_genex}"
      DEPFILE_NO_GENEX_PATH "${_vtk_javascript_depfile_nogenex}"
      TOOL_FLAGS "-MF")

    add_custom_command(
      OUTPUT  "${_vtk_javascript_source_output}"
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
              "$<TARGET_FILE:${_vtk_javascript_wrap_target}>"
              ${_vtk_javascript_depfile_flags}
              "@${_vtk_javascript_args_file}"
              -o "${_vtk_javascript_source_output}"
              "${_vtk_javascript_header}"
              ${_vtk_javascript_macros_args}
      ${_vtk_javascript_depfile_args}
      COMMENT "Generating JavaScript wrapper sources for ${_vtk_javascript_basename}"
      DEPENDS
        "${_vtk_javascript_header}"
        "${_vtk_javascript_args_file}"
        "$<TARGET_FILE:${_vtk_javascript_wrap_target}>"
        ${_vtk_javascript_command_depends})
  endforeach ()

  set("${sources}"
    "${_vtk_javascript_library_sources}"
    PARENT_SCOPE)
  set("${classes}"
    "${_vtk_javascript_library_classes}"
    PARENT_SCOPE)
endfunction ()

#[==[.rst:

.. cmake:command:: _vtk_module_wrap_javascript_library


  Generate a JavaScript library for a set of modules. |module-impl|

  A JavaScript module library may consist of the JavaScript wrappings of multiple
  modules. This is useful for kit-based builds where the modules part of the same
  kit belong to the same JavaScript module as well.

    .. code-block:: cmake

      _vtk_module_wrap_javascript_library(<module> ...)

  The first argument is the name of the JavaScript module. The remaining arguments
  are modules to include in the JavaScript module.

  The remaining information it uses is assumed to be provided by the
  :cmake:command:`vtk_module_wrap_javascript function`.
#]==]
function (_vtk_module_wrap_javascript_library module sources classes)
  # Generate embind C++ source code
  _vtk_module_wrap_javascript_sources("${module}" _vtk_javascript_sources _vtk_javascript_classes)
  set("${sources}"
    "${_vtk_javascript_sources}"
    PARENT_SCOPE)
  set("${classes}"
    "${_vtk_javascript_classes}"
    PARENT_SCOPE)
endfunction ()


#[==[.rst:
.. cmake:command:: vtk_module_wrap_javascript

  Wrap a set of modules for use in JavaScript.|module-wrapping-javascript|
  
  .. code-block:: cmake
  
     vtk_module_wrap_javascript(
       MODULES <module>...
       TARGET_NAME <name>
       [WRAPPED_MODULES <varname>]
       [UTILITY_TARGET <target>]
       [EXTRA_BINDING_SOURCES <sources_list>]      
       [MODULE_EXPORT_NAME <name>]
       [INSTALL_EXPORT <export>]
       [COMPONENT <component>]
       [BINDING_OBJECTS_DESTINATION <destination>]
       [DEBUG_INFO <debug>]
       [OPTIMIZATION <optimization>]
       [MEMORY64 <ON|OFF>]
     )
    
  * ``MODULES``: (Required) The list of modules to wrap.
  * ``TARGET_NAME``: (Required) The name of the generated js/wasm files.
  * ``WRAPPED_MODULES``: (Recommended) Not all modules are wrappable. This
    variable will be set to contain the list of modules which were wrapped.
  * ``UTILITY_TARGET``: If specified, all libraries made by the JavaScript wrapping
    will link privately to this target. This may be used to add compile flags
    to the JavaScript libraries.
  * ``EXTRA_BINDING_SOURCES``: Optional list of emscripten bindings sources to
    append to the list of automatically wrapped sources.
  * ``MODULE_EXPORT_NAME``: Optional name for the async function called to instantiate
    the wasm module. Sets the value of the emscripten EXPORT_NAME variable which will
    default to 'Module' if not specified.
  * ``INSTALL_EXPORT``: If provided, installed targets are added to the provided export set.
  * ``COMPONENT``: Installation component of the install rules created by this function.
    Defaults to ``development``.
  * ``BINDING_OBJECTS_DESTINATION``: Optional install destination for the objects libraries that
    contain the generated binding sources. Default value: CMAKE_INSTALL_LIBDIR.
  * ``DEBUG_INFO``: (Recommended) Extent of debug information in webassembly binaries:
    - NONE:         -g0
    - READABLE_JS:  -g1
    - PROFILE:      -g2
    - DEBUG_NATIVE: -g3 with emscripten compiler setting ASSERTIONS set to 1.
    Default value: "NONE".
  * ``OPTIMIZATION``: (Recommended) Optimization knobs for the webassembly binaries:
    - NO_OPTIMIZATION:       -O0
    - LITTLE:                -O1
    - MORE:                  -O2
    - BEST:                  -O3
    - SMALL:                 -Os
    - SMALLEST:              -Oz
    - SMALLEST_WITH_CLOSURE: -Oz with --closure 1.
    Default value: "SMALL".
  * ``MEMORY64``: (Recommended) The architecture to compile for.
    Sets the value of the emscripten MEMORY64 compiler setting.
    ``OFF`` is wasm32 and ``ON`` is wasm64.
    Defaults to ``OFF``

  The set of modules is compiled to a single ``<TARGET_NAME>.wasm/.js`` file.
  
  For each wrapped module, a ``vtk<module>WebObjects`` object library will be created.
  If the object library already exists for a given module, $<TARGET_OBJECTS:...> is used to
  automatically retrieve the list of pregenerated wrapped sources.
  Pregenerated sources can be filtered by specifying a list of class names in the following target
  properties prior to calling this macro:
  - vtk_module_wrap_javascript_exclude: Exclude the specified list of class names
  - vtk_module_wrap_javascript_include: Only include the specified list of class names 
#]==]
function (vtk_module_wrap_javascript)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_javascript
  ""
  "TARGET_NAME;WRAPPED_MODULES;UTILITY_TARGET;MODULE_EXPORT_NAME;INSTALL_EXPORT;COMPONENT;BINDING_OBJECTS_DESTINATION;DEBUG_INFO;OPTIMIZATION;MEMORY64"
  "MODULES;EXTRA_BINDING_SOURCES")

  if (_vtk_javascript_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_wrap_javascript: "
      "${_vtk_javascript_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_javascript_MODULES)
    message(WARNING
      "No modules were requested for java wrapping.")
    return ()
  endif ()

  if (NOT _vtk_javascript_TARGET_NAME)
    message(FATAL_ERROR
      "vtk_module_wrap_javascript: No output target name provided for the generated js/wasm files")
    return ()
  endif ()

  if (NOT DEFINED _vtk_javascript_COMPONENT)
    set(_vtk_javascript_COMPONENT "development")
  endif ()

  if (NOT DEFINED _vtk_javascript_BINDING_OBJECTS_DESTINATION)
    set(_vtk_javascript_BINDING_OBJECTS_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  endif ()

  if (NOT DEFINED _vtk_javascript_DEBUG_INFO)
    set(_vtk_javascript_DEBUG_INFO "NONE")
  endif ()
  set(_vtk_javascript_known_DEBUG_INFO
    "NONE"
    "READABLE_JS"
    "PROFILE"
    "DEBUG_NATIVE")
  if (NOT _vtk_javascript_DEBUG_INFO IN_LIST _vtk_javascript_known_DEBUG_INFO)
    message(WARNING
      "Unknown debug info '${_vtk_javascript_DEBUG_INFO}'. Using emcc without debug information.")
    set(_vtk_javascript_OPTIMIZATION "NONE")
  endif ()

  if (NOT DEFINED _vtk_javascript_OPTIMIZATION)
    set(_vtk_javascript_OPTIMIZATION "SMALL")
  endif ()
  set(_vtk_javascript_known_OPTIMIZATION
    "NO_OPTIMIZATION"
    "LITTLE"
    "MORE"
    "BEST"
    "SMALL"
    "SMALLEST"
    "SMALLEST_WITH_CLOSURE")
  if (NOT _vtk_javascript_OPTIMIZATION IN_LIST _vtk_javascript_known_OPTIMIZATION)
    message(WARNING
      "Unknown optimization level '${_vtk_javascript_OPTIMIZATION}'. "
      "Using emcc without optimizations.")
    set(_vtk_javascript_OPTIMIZATION "NO_OPTIMIZATION")
  endif ()

  if (NOT DEFINED _vtk_javascript_MEMORY64)
    set(_vtk_javascript_MEMORY64 OFF)
  endif ()

  set(_vtk_javascript_binding_sources)
  set(_vtk_javascript_binding_classes)
  
  include(vtkModuleWrapJavaScriptExclusions OPTIONAL)

  foreach (_vtk_javascript_module IN LISTS _vtk_javascript_MODULES)
    # Can the module ever be wrapped?
    _vtk_module_get_module_property("${_vtk_javascript_module}"
      PROPERTY  "exclude_wrap"
      VARIABLE  _vtk_javascript_exclude_wrap)
    if (_vtk_javascript_exclude_wrap)
      continue ()
    endif ()
    
    # Generate binding source code
    _vtk_module_get_module_property(${_vtk_javascript_module}
      PROPERTY "library_name"
      VARIABLE _module_library_name)
    set(_vtk_javascript_module_objects "${_module_library_name}WebObjects")
    if (TARGET "${_vtk_javascript_module_objects}")
      # Use precompiled binding sources
      set(_binding_sources "$<TARGET_OBJECTS:${_vtk_javascript_module_objects}>")
      # Filter sources with exclusion list
      get_target_property(_module_wrapexclude "${_vtk_javascript_module_objects}" vtk_module_wrap_javascript_exclude)
      if (_module_wrapexclude)
        list(JOIN _module_wrapexclude "|" _exclude_headers)
        set(_binding_sources "$<FILTER:${_binding_sources},EXCLUDE,(${_exclude_headers})Embinding.cxx>")
      endif ()
      # Filter sources with inclusion list
      get_target_property(_module_wrapinclude "${_vtk_javascript_module_objects}" vtk_module_wrap_javascript_include)
      if (_module_wrapinclude)
        list(JOIN _module_wrapinclude "|" _include_headers)
        set(_binding_sources "$<FILTER:${_binding_sources},INCLUDE,(${_include_headers})Embinding.cxx>")
      endif ()
      list(APPEND _vtk_javascript_binding_sources "${_binding_sources}")
    else ()
      # Generate binding sources
      _vtk_module_wrap_javascript_library("${_vtk_javascript_module}" _vtk_javascript_library_binding_sources _vtk_javascript_library_binding_classes)
      list(APPEND _vtk_javascript_binding_sources
        ${_vtk_javascript_library_binding_sources})
      list(APPEND _vtk_javascript_binding_classes
        ${_vtk_javascript_library_binding_classes})
      if (NOT _vtk_javascript_library_binding_sources)
        continue ()
      endif ()

      # Add object library to compile generated binding sources
      add_library("${_vtk_javascript_module_objects}" OBJECT 
        ${_vtk_javascript_library_binding_sources})

      target_link_libraries("${_vtk_javascript_module_objects}"
        PRIVATE
          VTK::WrappingJavaScript # For vtkEmbindSmartPointerTrait.h
          "${_vtk_javascript_module}")

      vtk_module_autoinit(
        MODULES ${_vtk_javascript_module}
        TARGETS "${_vtk_javascript_module_objects}")

      # Get link dependencies    
      _vtk_module_get_module_property("${_vtk_javascript_module}"
        PROPERTY  "private_depends"
        VARIABLE  _vtk_javascript_module_private_depends)

      _vtk_module_get_module_property("${_vtk_javascript_module}"
        PROPERTY  "depends"
        VARIABLE  _vtk_javascript_module_depends)

      target_link_libraries("${_vtk_javascript_module_objects}"
        PRIVATE
          ${_vtk_javascript_module_private_depends}
          ${_vtk_javascript_module_depends})

      # Export object library
      if (_vtk_javascript_INSTALL_EXPORT)
        install(
          TARGETS "${_vtk_javascript_module_objects}"
          EXPORT  "${_vtk_javascript_INSTALL_EXPORT}"
          OBJECTS DESTINATION "${_vtk_javascript_BINDING_OBJECTS_DESTINATION}")
      endif ()
    endif ()

    # Store the modules that have been wrapped
    list(APPEND _vtk_javascript_all_wrapped_modules
      "${_vtk_javascript_module}")

  endforeach ()

  if (NOT _vtk_javascript_binding_sources)
    return ()
  endif ()

  list(APPEND _vtk_javascript_binding_sources ${_vtk_javascript_EXTRA_BINDING_SOURCES})

  # Build <TARGET_NAME>.[js, wasm]
  set(_vtk_javascript_target "${_vtk_javascript_TARGET_NAME}")
  add_executable("${_vtk_javascript_target}"
    ${_vtk_javascript_binding_sources})
  if (_vtk_javascript_UTILITY_TARGET)
    target_link_libraries("${_vtk_javascript_target}"
      PRIVATE
        "${_vtk_javascript_UTILITY_TARGET}")
  endif ()

  vtk_module_autoinit(
    MODULES ${_vtk_javascript_MODULES}
    TARGETS "${_vtk_javascript_target}")

  target_link_libraries("${_vtk_javascript_target}"
    PRIVATE
      VTK::WrappingJavaScript
      ${_vtk_javascript_MODULES})
  
  if (_vtk_javascript_INSTALL_EXPORT)
    install(
      TARGETS "${_vtk_javascript_target}"
      EXPORT  "${_vtk_javascript_INSTALL_EXPORT}"
      COMPONENT "${_vtk_javascript_COMPONENT}")

    # Install wasm file next to the corresponding js file
    # https://gitlab.kitware.com/cmake/cmake/-/issues/20745
    install(FILES
      "$<TARGET_FILE_DIR:${_vtk_javascript_target}>/$<TARGET_FILE_BASE_NAME:${_vtk_javascript_target}>.wasm"
      TYPE BIN
      COMPONENT "${_vtk_javascript_COMPONENT}")
  endif ()

  list(APPEND emscripten_link_options
    "-lembind"
    "-sWASM=1"
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sEXPORTED_RUNTIME_METHODS=['ENV', 'FS', 'ccall', 'stringToNewUTF8', 'addFunction']"
    "-sEXPORTED_FUNCTIONS=['_free', '_malloc']"
    "-sINCLUDE_FULL_LIBRARY" # for addFunction
    "-sALLOW_TABLE_GROWTH=1"
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
  )

  if (_vtk_javascript_MODULE_EXPORT_NAME)
    list(APPEND emscripten_link_options "-sEXPORT_NAME=${_vtk_javascript_MODULE_EXPORT_NAME}")
  endif ()

  # TODO: Move these to vtkCompilerPlatformFlags? Options must be applied on the vtk c++ translation units too.
  set(emscripten_debug_options)
  set(emscripten_optimizations)

  if (NOT _vtk_javascript_MEMORY64)
    list(APPEND emscripten_compile_options "-sMEMORY64=0")
    list(APPEND emscripten_link_options "-sMEMORY64=0")
  else ()
    list(APPEND emscripten_compile_options "-sMEMORY64=1")
    list(APPEND emscripten_link_options "-sMEMORY64=1")
    list(APPEND emscripten_link_options "-sWASM_BIGINT=1")
  endif ()

  if (_vtk_javascript_DEBUG_INFO STREQUAL "NONE")
    list(APPEND emscripten_debug_options
      "-g0")
  elseif (_vtk_javascript_DEBUG_INFO STREQUAL "READABLE_JS")
    list(APPEND emscripten_debug_options
      "-g1")
  elseif (_vtk_javascript_DEBUG_INFO STREQUAL "PROFILE")
    list(APPEND emscripten_debug_options
      "-g2")
  elseif (_vtk_javascript_DEBUG_INFO STREQUAL "DEBUG_NATIVE")
    list(APPEND emscripten_debug_options
      "-g3")
    list(APPEND emscripten_link_options
      "-sASSERTIONS=1")
  else ()
    # internal error; this is to catch problems with the filtering above
    message(FATAL_ERROR "Unrecognized setting for _vtk_javascript_DEBUG_INFO")
  endif ()

  if (_vtk_javascript_OPTIMIZATION STREQUAL "NO_OPTIMIZATION")
    list(APPEND emscripten_optimizations
      "-O0")
  elseif (_vtk_javascript_OPTIMIZATION STREQUAL "LITTLE")
    list(APPEND emscripten_optimizations
      "-O1")
  elseif (_vtk_javascript_OPTIMIZATION STREQUAL "MORE")
    list(APPEND emscripten_optimizations
      "-O2")
  elseif (_vtk_javascript_OPTIMIZATION STREQUAL "BEST")
    list(APPEND emscripten_optimizations
      "-O3")
  elseif (_vtk_javascript_OPTIMIZATION STREQUAL "SMALL")
    list(APPEND emscripten_optimizations
      "-Os")
  elseif (_vtk_javascript_OPTIMIZATION STREQUAL "SMALLEST")
    list(APPEND emscripten_optimizations
      "-Oz")
  elseif (_vtk_javascript_OPTIMIZATION STREQUAL "SMALLEST_WITH_CLOSURE")
    list(APPEND emscripten_optimizations
      "-Oz")
    list(APPEND emscripten_link_options
      "--closure 1")
  else ()
    # internal error; this is to catch problems with the filtering above
    message(FATAL_ERROR "Unrecognized setting for _vtk_javascript_OPTIMIZATION")
  endif ()

  target_compile_options("${_vtk_javascript_target}"
    PRIVATE
      ${emscripten_compile_options}
      ${emscripten_optimizations}
      ${emscripten_debug_options})

  target_link_options("${_vtk_javascript_target}"
    PRIVATE
      ${emscripten_link_options}
      ${emscripten_optimizations}
      ${emscripten_debug_options})

  if (NOT _vtk_javascript_all_wrapped_modules)
    message(FATAL_ERROR
      "None of the given modules could be wrapped.")
  endif ()

  if (DEFINED _vtk_javascript_WRAPPED_MODULES)
    set("${_vtk_javascript_WRAPPED_MODULES}"
      "${_vtk_javascript_all_wrapped_modules}"
      PARENT_SCOPE)
  endif ()
endfunction ()
