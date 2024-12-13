#[==[.rst:
*****************
vtkModuleWrapJava
*****************
#]==]

#[==[.rst:
APIs for wrapping modules for Java
#]==]

#[==[.rst:

.. cmake:command:: _vtk_module_wrap_java_sources

  Generate sources for using a module's classes from Java. |module-impl|

  This function generates the wrapped sources for a module. It places the list of
  generated source files and Java source files in variables named in the second
  and third arguments, respectively.

  .. code-block:: cmake

    _vtk_module_wrap_java_sources(<module> <sources> <classes>)
#]==]

cmake_policy(PUSH)
cmake_policy(SET CMP0053 NEW)

function (_vtk_module_wrap_java_sources module sources java_sources)
  _vtk_module_get_module_property("${module}"
    PROPERTY  "exclude_wrap"
    VARIABLE  _vtk_java_exclude_wrap)
  if (_vtk_java_exclude_wrap)
    return ()
  endif ()

  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_java_library_name}Java")

  set(_vtk_java_args_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_java_library_name}Java/${_vtk_java_library_name}-java.$<CONFIGURATION>.args")
  set(_vtk_java_init_data_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_java_library_name}Java/${_vtk_java_library_name}-java-init.data")

  set(_vtk_java_hierarchy_depends "${module}")
  _vtk_module_get_module_property("${module}"
    PROPERTY  "private_depends"
    VARIABLE  _vtk_java_private_depends)
  list(APPEND _vtk_java_hierarchy_depends
    ${_vtk_java_private_depends})
  _vtk_module_get_module_property("${module}"
    PROPERTY  "optional_depends"
    VARIABLE  _vtk_java_optional_depends)
  foreach (_vtk_java_optional_depend IN LISTS _vtk_java_optional_depends)
    if (TARGET "${_vtk_java_optional_depend}")
      list(APPEND _vtk_java_hierarchy_depends
        "${_vtk_java_optional_depend}")
    endif ()
  endforeach ()

  set(_vtk_java_command_depends)
  foreach (_vtk_java_hierarchy_depend IN LISTS _vtk_java_hierarchy_depends)
    _vtk_module_get_module_property("${_vtk_java_hierarchy_depend}"
      PROPERTY  "hierarchy"
      VARIABLE  _vtk_java_hierarchy_file)
    if (_vtk_java_hierarchy_file)
      list(APPEND _vtk_java_hierarchy_files "${_vtk_java_hierarchy_file}")
      get_property(_vtk_java_is_imported
        TARGET    "${_vtk_java_hierarchy_depend}"
        PROPERTY  "IMPORTED")
      if (_vtk_java_is_imported OR CMAKE_GENERATOR MATCHES "Ninja")
        list(APPEND _vtk_java_command_depends "${_vtk_java_hierarchy_file}")
      else ()
        _vtk_module_get_module_property("${_vtk_java_hierarchy_depend}"
          PROPERTY  "library_name"
          VARIABLE  _vtk_java_hierarchy_library_name)
        if (TARGET "${_vtk_java_hierarchy_library_name}-hierarchy")
          list(APPEND _vtk_java_command_depends "${_vtk_java_hierarchy_library_name}-hierarchy")
        else ()
          message(FATAL_ERROR
            "The ${_vtk_java_hierarchy_depend} hierarchy file is attached to a non-imported target "
            "and a hierarchy target (${_vtk_java_hierarchy_library_name}-hierarchy) is "
            "missing.")
        endif ()
      endif ()
    endif ()
  endforeach ()

  set(_vtk_java_genex_allowed 1)
  if (CMAKE_VERSION VERSION_LESS "3.19")
    get_property(_vtk_java_target_type
      TARGET   "${_vtk_java_target_name}"
      PROPERTY TYPE)
    if (_vtk_java_target_type STREQUAL "INTERFACE_LIBRARY")
      set(_vtk_java_genex_allowed 0)
    endif ()
  endif ()

  set(_vtk_java_genex_compile_definitions "")
  set(_vtk_java_genex_include_directories "")
  if (_vtk_java_genex_allowed)
    set(_vtk_java_genex_compile_definitions
      "$<TARGET_PROPERTY:${_vtk_java_target_name},COMPILE_DEFINITIONS>")
    set(_vtk_java_genex_include_directories
      "$<TARGET_PROPERTY:${_vtk_java_target_name},INCLUDE_DIRECTORIES>")
  else ()
    if (NOT DEFINED ENV{CI})
      message(AUTHOR_WARNING
        "Java wrapping is not using target-local compile definitions or "
        "include directories. This may affect generation of the Java wrapper "
        "sources for the ${module} module. Use CMake 3.19+ to guarantee "
        "intended behavior.")
    endif ()
  endif ()
  file(GENERATE
    OUTPUT  "${_vtk_java_args_file}"
    CONTENT "$<$<BOOL:${_vtk_java_genex_compile_definitions}>:\n-D\'$<JOIN:${_vtk_java_genex_compile_definitions},\'\n-D\'>\'>\n
$<$<BOOL:${_vtk_java_genex_include_directories}>:\n-I\'$<JOIN:${_vtk_java_genex_include_directories},\'\n-I\'>\'>\n
$<$<BOOL:${_vtk_java_hierarchy_files}>:\n--types \'$<JOIN:${_vtk_java_hierarchy_files},\'\n--types \'>\'>\n")

  set(_vtk_java_sources)
  set(_vtk_java_java_sources)

  _vtk_module_get_module_property("${module}"
    PROPERTY  "headers"
    VARIABLE  _vtk_java_headers)
  set(_vtk_java_classes)
  foreach (_vtk_java_header IN LISTS _vtk_java_headers)
    get_filename_component(_vtk_java_basename "${_vtk_java_header}" NAME_WE)
    list(APPEND _vtk_java_classes
      "${_vtk_java_basename}")

    # The vtkWrapJava tool has special logic for the `vtkRenderWindow` class.
    # This extra logic requires its wrappers to be compiled as ObjC++ code
    # instead.
    set(_vtk_java_ext "cxx")
    if (APPLE AND _vtk_java_basename STREQUAL "vtkRenderWindow")
      set(_vtk_java_ext "mm")
    endif ()

    set(_vtk_java_source_output
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_java_library_name}Java/${_vtk_java_basename}Java.${_vtk_java_ext}")
    set(_vtk_java_wrap_depfile_genex
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_java_library_name}Java/${_vtk_java_basename}Java.${_vtk_java_ext}.$<CONFIG>.d")
    set(_vtk_java_wrap_depfile_nogenex
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_java_library_name}Java/${_vtk_java_basename}Java.${_vtk_java_ext}.d")
    list(APPEND _vtk_java_sources
      "${_vtk_java_source_output}")

    set(_vtk_java_wrap_target "VTK::WrapJava")
    set(_vtk_java_macros_args)
    if (TARGET VTKCompileTools::WrapJava)
      set(_vtk_java_wrap_target "VTKCompileTools::WrapJava")
      if (TARGET VTKCompileTools_macros)
        list(APPEND _vtk_java_command_depends
          "VTKCompileTools_macros")
        list(APPEND _vtk_java_macros_args
          -undef
          -imacros "${_VTKCompileTools_macros_file}")
      endif ()
    endif ()

    set(_vtk_java_parse_target "VTK::ParseJava")
    if (TARGET VTKCompileTools::ParseJava)
      set(_vtk_java_parse_target "VTKCompileTools::ParseJava")
    endif ()

    _vtk_module_depfile_args(
      MULTI_CONFIG_NEEDS_GENEX
      TOOL_ARGS _vtk_java_depfile_flags
      CUSTOM_COMMAND_ARGS _vtk_java_depfile_args
      SOURCE "${_vtk_java_header}"
      DEPFILE_PATH "${_vtk_java_wrap_depfile_genex}"
      DEPFILE_NO_GENEX_PATH "${_vtk_java_wrap_depfile_nogenex}"
      TOOL_FLAGS "-MF")

    add_custom_command(
      OUTPUT  "${_vtk_java_source_output}"
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
              "$<TARGET_FILE:${_vtk_java_wrap_target}>"
              ${_vtk_java_depfile_flags}
              "@${_vtk_java_args_file}"
              -o "${_vtk_java_source_output}"
              "${_vtk_java_header}"
              ${_vtk_java_warning_args}
              ${_vtk_java_macros_args}
      ${_vtk_java_depfile_args}
      COMMENT "Generating Java wrapper sources for ${_vtk_java_basename}"
      DEPENDS
        "${_vtk_java_header}"
        "${_vtk_java_args_file}"
        "$<TARGET_FILE:${_vtk_java_wrap_target}>"
        ${_vtk_java_command_depends})

    set(_vtk_java_java_source_output
      "${_vtk_java_JAVA_OUTPUT}/${_vtk_java_basename}.java")
    set(_vtk_java_parse_depfile
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_java_library_name}Java/${_vtk_java_basename}.java.d")
    list(APPEND _vtk_java_java_sources
      "${_vtk_java_java_source_output}")

    _vtk_module_depfile_args(
      TOOL_ARGS _vtk_java_depfile_flags
      CUSTOM_COMMAND_ARGS _vtk_java_depfile_args
      SOURCE "${_vtk_java_header}"
      DEPFILE_PATH "${_vtk_java_parse_depfile}"
      TOOL_FLAGS "-MF")

    add_custom_command(
      OUTPUT  "${_vtk_java_java_source_output}"
      COMMAND "${_vtk_java_parse_target}"
              ${_vtk_java_depfile_flags}
              "@${_vtk_java_args_file}"
              -o "${_vtk_java_java_source_output}"
              "${_vtk_java_header}"
              ${_vtk_java_warning_args}
              ${_vtk_java_macros_args}
      ${_vtk_java_depfile_args}
      COMMENT "Generating Java sources for ${_vtk_java_basename}"
      DEPENDS
        "${_vtk_java_header}"
        "${_vtk_java_args_file}"
        "$<TARGET_FILE:${_vtk_java_parse_target}>"
        ${_vtk_java_command_depends})
  endforeach ()

  set("${sources}"
    "${_vtk_java_sources}"
    PARENT_SCOPE)

  set("${java_sources}"
    "${_vtk_java_java_sources}"
    PARENT_SCOPE)
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_wrap_java_library

  Generate a JNI library for a set of modules. |module-impl|

  A single JNI library may consist of the Java wrappings of multiple modules.
  This is useful for kit-based builds where the modules part of the same kit
  belong to the same JNI library as well.

  .. code-block:: cmake

    _vtk_module_wrap_java_library(<name> <module>...)

  The first argument is the name of the JNI library. The remaining arguments are
  modules to include in the JNI library.

  The remaining information it uses is assumed to be provided by the
  :cmake:command:`vtk_module_wrap_java` function.
#]==]
function (_vtk_module_wrap_java_library name)
  set(_vtk_java_library_sources)
  set(_vtk_java_library_java_sources)
  set(_vtk_java_library_link_depends)
  foreach (_vtk_java_module IN LISTS ARGN)
    _vtk_module_get_module_property("${_vtk_java_module}"
      PROPERTY  "exclude_wrap"
      VARIABLE  _vtk_java_exclude_wrap)
    if (_vtk_java_exclude_wrap)
      continue ()
    endif ()
    _vtk_module_real_target(_vtk_java_target_name "${_vtk_java_module}")
    _vtk_module_get_module_property("${_vtk_java_module}"
      PROPERTY  "library_name"
      VARIABLE  _vtk_java_library_name)
    _vtk_module_wrap_java_sources("${_vtk_java_module}" _vtk_java_sources _vtk_java_java_sources)
    list(APPEND _vtk_java_library_sources
      ${_vtk_java_sources})
    list(APPEND _vtk_java_library_java_sources
      ${_vtk_java_java_sources})

    set_source_files_properties(${_vtk_java_java_sources} PROPERTIES GENERATED TRUE)

    _vtk_module_get_module_property("${_vtk_java_module}"
      PROPERTY  "depends"
      VARIABLE  _vtk_java_module_depends)
    _vtk_module_get_module_property("${_vtk_java_module}"
      PROPERTY  "private_depends"
      VARIABLE  _vtk_java_module_private_depends)
    _vtk_module_get_module_property("${_vtk_java_module}"
      PROPERTY  "optional_depends"
      VARIABLE  _vtk_java_module_optional_depends)
    _vtk_module_get_module_property("${_vtk_java_module}"
      PROPERTY  "implements"
      VARIABLE  _vtk_java_module_implements)

    list(APPEND _vtk_java_module_depends
      ${_vtk_java_module_private_depends}
      ${_vtk_java_module_implements})

    foreach (_vtk_java_optional_depend IN LISTS _vtk_java_module_optional_depends)
      if (TARGET "${_vtk_java_optional_depend}")
        list(APPEND _vtk_java_module_depends "${_vtk_java_optional_depend}")
      endif ()
    endforeach ()

    foreach (_vtk_java_module_depend IN LISTS _vtk_java_module_depends)
      # Remove self dependency, this is needed for self-implementing modules
      if (_vtk_java_module_depend STREQUAL _vtk_java_module)
        continue()
      endif()

      _vtk_module_get_module_property("${_vtk_java_module_depend}"
        PROPERTY  "exclude_wrap"
        VARIABLE  _vtk_java_module_depend_exclude_wrap)
      if (_vtk_java_module_depend_exclude_wrap)
        continue ()
      endif ()

      _vtk_module_get_module_property("${_vtk_java_module_depend}"
        PROPERTY  "library_name"
        VARIABLE  _vtk_java_depend_library_name)

      # XXX(kits): This doesn't work for kits.
      list(APPEND _vtk_java_library_link_depends
        "${_vtk_java_depend_library_name}Java")
    endforeach ()
  endforeach ()

  if (NOT _vtk_java_library_sources)
    return ()
  endif ()

  if (_vtk_java_library_link_depends)
    list(REMOVE_DUPLICATES _vtk_java_library_link_depends)
  endif ()

  set(_vtk_java_target "${name}Java")

  # XXX(java): Should this be a `MODULE`? If not, we should probably export
  # these targets, but then we'll need logic akin to the `vtkModuleWrapPython`
  # logic for loading wrapped modules from other packages.
  add_library("${_vtk_java_target}" SHARED)
  target_sources("${_vtk_java_target}"
    PRIVATE
      ${_vtk_java_library_sources})
  # Add a dummy file set to optimize dependencies. See CMP0154.
  _vtk_module_add_file_set("${_vtk_java_target}"
    BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}"
    NAME      dummy)

  if (_vtk_java_UTILITY_TARGET)
    target_link_libraries("${_vtk_java_target}"
      PRIVATE
        "${_vtk_java_UTILITY_TARGET}")
  endif ()

  add_custom_target("${_vtk_java_target}-java-sources"
    DEPENDS
      ${_vtk_java_library_java_sources})
  add_dependencies("${_vtk_java_target}"
    "${_vtk_java_target}-java-sources")
  if (MINGW)
    set_property(TARGET "${_vtk_java_target}"
      PROPERTY
        PREFIX "")
  endif ()
  set_property(TARGET "${_vtk_java_target}"
    PROPERTY
      "_vtk_module_java_files" "${_vtk_java_library_java_sources}")

  if (_vtk_java_JNILIB_DESTINATION)
    install(
      TARGETS "${_vtk_java_target}"
      # Windows
      RUNTIME
        DESTINATION "${_vtk_java_JNILIB_DESTINATION}"
        COMPONENT   "${_vtk_java_JNILIB_COMPONENT}"
      # Other platforms
      LIBRARY
        DESTINATION "${_vtk_java_JNILIB_DESTINATION}"
        COMPONENT   "${_vtk_java_JNILIB_COMPONENT}")
  endif ()

  vtk_module_autoinit(
    MODULES ${ARGN}
    TARGETS "${_vtk_java_target}")

  target_link_libraries("${_vtk_java_target}"
    PRIVATE
      ${ARGN}
      # XXX(java): If we use modules, remove this.
      ${_vtk_java_library_link_depends}
      VTK::Java)
endfunction ()

#[==[.rst:

.. cmake:command:: vtk_module_wrap_java

  Wrap a set of modules for use in Java. |module-wrapping-java|

  .. code-block:: cmake

    vtk_module_wrap_java(
      MODULES <module>...
      [WRAPPED_MODULES <varname>]

      [UTILITY_TARGET <target>]

      [JAVA_OUTPUT <destination>]

      [LIBRARY_DESTINATION <destination>]
      [JNILIB_DESTINATION <destination>]
      [JNILIB_COMPONENT <component>]

      [WARNINGS <warning>...])

  * ``MODULES``: (Required) The list of modules to wrap.
  * ``WRAPPED_MODULES``: (Recommended) Not all modules are wrappable. This
    variable will be set to contain the list of modules which were wrapped.
  * ``UTILITY_TARGET``: If specified, all libraries made by the Java wrapping
    will link privately to this target. This may be used to add compile flags
    to the Java libraries.
  * ``JAVA_OUTPUT``: Defaults to
    ``${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vtkJava``. Java source files are
    written to this directory. After generation, the files may be compiled as
    needed.
  * ``LIBRARY_DESTINATION`` (Recommended): If provided, dynamic loader
    information will be added to modules for loading dependent libraries.
  * ``JNILIB_DESTINATION``: Where to install JNI libraries.
  * ``JNILIB_COMPONENT``: Defaults to ``jni``. The install component to use for JNI
    libraries.
  * ``WARNINGS``: Warnings to enable. Supported warnings: ``empty``.

  For each wrapped module, a ``<module>Java`` target will be created. These targets
  will have a ``_vtk_module_java_files`` property which is the list of generated
  Java source files for that target.

  For dependency purposes, the ``<module>Java-java-sources`` target may also be
  used.
#]==]
function (vtk_module_wrap_java)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_java
    ""
    "JAVA_OUTPUT;WRAPPED_MODULES;LIBRARY_DESTINATION;JNILIB_DESTINATION;JNILIB_COMPONENT;UTILITY_TARGET"
    "MODULES;WARNINGS")

  if (_vtk_java_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_wrap_java: "
      "${_vtk_java_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_java_JAVA_OUTPUT)
    set(_vtk_java_JAVA_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vtkJava")
  endif ()

  if (NOT _vtk_java_JNILIB_COMPONENT)
    set(_vtk_java_JNILIB_COMPONENT "jni")
  endif ()

  set(_vtk_java_known_warnings
    empty)
  set(_vtk_java_warning_args)
  foreach (_vtk_java_warning IN LISTS _vtk_java_WARNINGS)
    if (NOT _vtk_java_warning IN_LIST _vtk_java_known_warnings)
      message(FATAL_ERROR
        "Unrecognized warning: ${_vtk_java_warning}")
    endif ()
    list(APPEND _vtk_java_warning_args
      "-W${_vtk_java_warning}")
  endforeach ()

  # Set up rpaths
  set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)
  if (UNIX)
    if (APPLE)
      set(_vtk_java_origin_rpath_prefix
        "@loader_path")
    else ()
      set(_vtk_java_origin_rpath_prefix
        "$ORIGIN")
    endif ()

    list(APPEND CMAKE_INSTALL_RPATH
      # For sibling wrapped modules.
      "${_vtk_java_origin_rpath_prefix}")

    if (DEFINED _vtk_java_LIBRARY_DESTINATION AND DEFINED _vtk_java_JNILIB_DESTINATION)
      file(RELATIVE_PATH _vtk_java_relpath
        "/prefix/${_vtk_java_JNILIB_DESTINATION}"
        "/prefix/${_vtk_java_LIBRARY_DESTINATION}")

      list(APPEND CMAKE_INSTALL_RPATH
        # For libraries.
        "${_vtk_java_origin_rpath_prefix}/${_vtk_java_relpath}")
    endif ()
  endif ()

  if (DEFINED _vtk_java_JNILIB_DESTINATION)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_vtk_java_JNILIB_DESTINATION}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_vtk_java_JNILIB_DESTINATION}")
  endif ()

  if (NOT _vtk_java_MODULES)
    message(WARNING
      "No modules were requested for java wrapping.")
    return ()
  endif ()

  # Disable CMake's automoc support for these targets.
  set(CMAKE_AUTOMOC 0)
  set(CMAKE_AUTORCC 0)
  set(CMAKE_AUTOUIC 0)

  set(_vtk_java_all_wrapped_modules)
  foreach (_vtk_java_module IN LISTS _vtk_java_MODULES)
    _vtk_module_get_module_property("${_vtk_java_module}"
      PROPERTY  "library_name"
      VARIABLE  _vtk_java_exclude_wrap)
    _vtk_module_get_module_property("${_vtk_java_module}"
      PROPERTY  "library_name"
      VARIABLE  _vtk_java_library_name)
    _vtk_module_wrap_java_library("${_vtk_java_library_name}" "${_vtk_java_module}")

    if (TARGET "${_vtk_java_library_name}Java")
      list(APPEND _vtk_java_all_wrapped_modules
        "${_vtk_java_module}")
    endif ()
  endforeach ()

  if (NOT _vtk_java_all_wrapped_modules)
    message(FATAL_ERROR
      "No modules given could be wrapped.")
  endif ()

  if (DEFINED _vtk_java_WRAPPED_MODULES)
    set("${_vtk_java_WRAPPED_MODULES}"
      "${_vtk_java_all_wrapped_modules}"
      PARENT_SCOPE)
  endif ()
endfunction ()

cmake_policy(POP)
