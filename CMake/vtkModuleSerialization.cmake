set(_vtkModuleSerialization_source_dir "${CMAKE_CURRENT_LIST_DIR}")

#[==[.rst:
**********************
vtkModuleSerialization
**********************

#]==]

get_filename_component(_vtkModuleSerialization_dir "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)

#[==[.rst:
.. cmake:command:: vtk_module_generate_library_serdes_registrar

  Generate functions that register (de)serialization functions for all classes
  in a library. |module-serialization|

  .. code-block:: cmake

    vtk_module_generate_library_serdes_registrar(
      MODULE             <module>
      EXPORT_MACRO_NAME  <export_macro_name>
      EXPORT_FILE_NAME   <export_file_name>
      REGISTRAR_HEADER   <registrar_header>
      REGISTRAR_SOURCE   <registrar_source>
      CLASSES            <class>...)

  Declares registrar function for ``MODULE`` in ``REGISTRAR_HEADER`` and
  writes implementation of the registrar in ``REGISTRAR_SOURCE``.

  * ``MODULE``: The name of a module.
  * ``EXPORT_MACRO_NAME``: The name of the export macro for ``MODULE``.
  * ``EXPORT_FILE_NAME``: The name of the header file which defines ``EXPORT_MACRO_NAME``
    for ``MODULE``.
  * ``REGISTRAR_HEADER``: This file will hold the declaration of registrar function for ``MODULE``.
  * ``REGISTRAR_SOURCE``: This file will contain the implementation of registrar function for ``MODULE``.
  * ``CLASSES``: List of classes that will be registered with (de)serializer in this library.

#]==]
function (vtk_module_generate_library_serdes_registrar)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_serdes
    ""
    "MODULE;EXPORT_MACRO_NAME;EXPORT_FILE_NAME;REGISTRAR_HEADER;REGISTRAR_SOURCE"
    "CLASSES")

  if (_vtk_serdes_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_generate_library_serdes_registrar: "
      "${_vtk_serdes_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_serdes_MODULE)
    message(FATAL_ERROR "No module was specified!")
  endif ()

  if (NOT _vtk_serdes_EXPORT_MACRO_NAME)
    message(FATAL_ERROR "No export macro name was specified!")
  endif ()

  if (NOT _vtk_serdes_EXPORT_FILE_NAME)
    message(FATAL_ERROR "No export file name was specified!")
  endif ()

  if (NOT _vtk_serdes_REGISTRAR_HEADER)
    message(FATAL_ERROR "No registrar header file name was specified!")
  endif ()

  if (NOT _vtk_serdes_REGISTRAR_SOURCE)
    message(FATAL_ERROR "No registrar source file name was specified!")
  endif ()

  if (NOT _vtk_serdes_CLASSES)
    message(FATAL_ERROR "No classes were specified!")
  endif ()

  get_filename_component(_vtk_serdes_library_registrar_header "${_vtk_serdes_REGISTRAR_HEADER}" NAME)
  _vtk_module_get_module_property("${_vtk_serdes_MODULE}"
    PROPERTY  "library_name"
    VARIABLE  _vtk_serdes_library_name)

  set(_vtk_serdes_library "${_vtk_serdes_library_name}")
  set(_vtk_serdes_module_export_header "${_vtk_serdes_EXPORT_FILE_NAME}")
  set(_vtk_serdes_module_export_macro "${_vtk_serdes_EXPORT_MACRO_NAME}")
  set(_vtk_serdes_register_classes "")
  set(_vtk_serdes_register_classes_decls "")
  foreach (_vtk_serdes_class IN LISTS _vtk_serdes_CLASSES)
    string(APPEND _vtk_serdes_register_classes_decls
      "  int RegisterHandlers_${_vtk_serdes_class}SerDes(void* serializer, void* deserializer, void* invoker);\n")
    string(APPEND _vtk_serdes_register_classes "
  if(!RegisterHandlers_${_vtk_serdes_class}SerDes(serializer, deserializer, invoker))
  {
    *error = \"Failed to register handlers for ${_vtk_serdes_class}\";
    return FAIL;
  }\n")
  endforeach ()
  configure_file(
    "${_vtkModuleSerialization_source_dir}/vtkSerializationLibraryRegistrar.h.in"
    "${_vtk_serdes_REGISTRAR_HEADER}"
    @ONLY)
  configure_file(
    "${_vtkModuleSerialization_source_dir}/vtkSerializationLibraryRegistrar.cxx.in"
    "${_vtk_serdes_REGISTRAR_SOURCE}"
    @ONLY)
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_generate_libraries_serdes_registrar

  Generate functions that register (de)serialization handlers for a
  collection of modules. |module-serialization|

  .. code-block:: cmake

    vtk_module_generate_libraries_serdes_registrar(
      REGISTRAR_NAME     <registrar_name>
      REGISTRAR_SOURCE   <registrar_source>
      [MANDATORY_MODULES  <module>...]
      [OPTIONAL_MODULES   <module>...])

  Invokes registrar functions for all modules from ``OPTIONAL_MODULES`` and ``MANDATORY_MODULES``.
  Code is generated in ``REGISTRAR_SOURCE``.

  * ``REGISTRAR_NAME``: The name of the registrar function.
  * ``REGISTRAR_SOURCE``: This file will contain the implementation of registrar
    function which registers handlers for all ``MANDATORY_MODULES`` and ``OPTIONAL_MODULES``.
  * ``MANDATORY_MODULES``:  A list of ``PUBLIC`` or ``PRIVATE`` dependencies
    of the module which ``REGISTRAR_SOURCE`` is compiled into.
  * ``OPTIONAL_MODULES``: A list of optional dependencies of the module
    which ``REGISTRAR_SOURCE`` is compiled into.

#]==]
function (vtk_module_generate_libraries_serdes_registrar)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_serdes
    ""
    "REGISTRAR_NAME;REGISTRAR_SOURCE"
    "MANDATORY_MODULES;OPTIONAL_MODULES;")

  if (_vtk_serdes_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_generate_libraries_serdes_registrar: "
      "${_vtk_serdes_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_serdes_REGISTRAR_NAME)
    message(FATAL_ERROR "No registrar name was specified!")
  endif ()

  if (NOT _vtk_serdes_REGISTRAR_SOURCE)
    message(FATAL_ERROR "No registrar source file was specified!")
  endif ()

  set(_vtk_serdes_registrar_name "${_vtk_serdes_REGISTRAR_NAME}")
  set(_vtk_serdes_marshal_depends)
  list(APPEND _vtk_serdes_marshal_depends
    ${_vtk_serdes_MANDATORY_MODULES})
  foreach (_vtk_serdes_optional_depend IN LISTS _vtk_serdes_OPTIONAL_MODULES)
    _vtk_module_optional_dependency_exists("${_vtk_serdes_optional_depend}"
      SATISFIED_VAR _vtk_serdes_optional_depend_exists)
    if (NOT _vtk_serdes_optional_depend_exists)
      continue ()
    endif ()
    list(APPEND _vtk_serdes_marshal_depends
      "${_vtk_serdes_optional_depend}")
  endforeach ()

  set(_vtk_serdes_include_libraries_registrar_headers "")
  set(_vtk_serdes_register_libraries "")
  foreach (_vtk_serdes_module IN LISTS _vtk_serdes_marshal_depends)
    _vtk_module_get_module_property("${_vtk_serdes_module}"
      PROPERTY "library_name"
      VARIABLE _vtk_serdes_library_name)
    string(APPEND _vtk_serdes_include_libraries_registrar_headers
      "#include \"${_vtk_serdes_library_name}SerDes.h\"\n")
    string(APPEND _vtk_serdes_register_libraries "
  if(!RegisterClasses_${_vtk_serdes_library_name}(serializer, deserializer, invoker, error))
  {
    return FAIL;
  }\n")
  endforeach ()

  configure_file(
    "${_vtkModuleSerialization_source_dir}/vtkSerializationLibrariesRegistrar.cxx.in"
    "${_vtk_serdes_REGISTRAR_SOURCE}"
    @ONLY)
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_serdes_generate_sources

  Generate (de)serialization functions for classes that belong to
  ``MODULE`` in ``SERDES_SOURCES``. |module-serialization|

  .. code-block:: cmake

    _vtk_module_serdes_generate_sources(
      MODULE              <module>
      SERIALIZED_CLASSES  <serialized_classes>
      SERDES_SOURCES      <serdes_sources>)

  * ``MODULE``: Generates serialization sources for all classes in ``MODULE``
  * ``SERIALIZED_CLASSES``: This variable will contain the list of all classes that
    were serialized.
  * ``SERDES_SOURCES``: This variable will contain the list of all sources that
    were generated for ``SERIALIZED_CLASSES`` in ``MODULE``.

#]==]
function (_vtk_module_serdes_generate_sources)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_serdes
    ""
    "MODULE;SERIALIZED_CLASSES;SERDES_SOURCES"
    "")

  if (_vtk_serdes_UNPARSED_ARGUMENTS)
    message (FATAL_ERROR
      "Unparsed arguments for _vtk_module_serdes_generate_sources: "
      "${_vtk_serdes_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_serdes_MODULE)
    message(FATAL_ERROR "No module was specified!")
  endif ()

  if (NOT _vtk_serdes_SERIALIZED_CLASSES)
    message(FATAL_ERROR "No output variable for serialized classes was specified!")
  endif ()

  if (NOT _vtk_serdes_SERDES_SOURCES)
    message(FATAL_ERROR "No output variable for serialization and deserialization sources were specified")
  endif ()

  _vtk_module_get_module_property("${_vtk_serdes_MODULE}"
    PROPERTY "library_name"
    VARIABLE _vtk_serdes_library_name)
  set(_vtk_serdes_args_file "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_serdes_library_name}-SerDes.$<CONFIG>.args")
  set(_vtk_serdes_hierarchy_depends "${_vtk_serdes_MODULE}")
  # Get public dependencies of `module`.
  _vtk_module_get_module_property("${_vtk_serdes_MODULE}"
    PROPERTY  "public_depends"
    VARIABLE  _vtk_serdes_public_depends)
  list(APPEND _vtk_serdes_hierarchy_depends
    ${_vtk_serdes_public_depends})
  # Get private dependencies of `module`.
  _vtk_module_get_module_property("${_vtk_serdes_MODULE}"
    PROPERTY  "private_depends"
    VARIABLE  _vtk_serdes_private_depends)
  list(APPEND _vtk_serdes_hierarchy_depends
    ${_vtk_serdes_private_depends})
  # Get optional dependencies of `module`.
  _vtk_module_get_module_property("${_vtk_serdes_MODULE}"
    PROPERTY  "optional_depends"
    VARIABLE  _vtk_serdes_optional_depends)
  # Appended into hierarchy depends only if those optional dependencies are declared as targets
  foreach (_vtk_serdes_optional_depend IN LISTS _vtk_serdes_optional_depends)
    _vtk_module_optional_dependency_exists("${_vtk_serdes_optional_depend}"
      SATISFIED_VAR _vtk_serdes_optional_depend_exists)
    if (_vtk_serdes_optional_depend_exists)
      list(APPEND _vtk_serdes_hierarchy_depends
        "${_vtk_serdes_optional_depend}")
    endif ()
  endforeach ()
  # Add hierarchy targets or files as dependencies to the vtkSerDes command.
  set(_vtk_serdes_command_depends)
  set(_vtk_serdes_hierarchy_files)
  foreach (_vtk_serdes_hierarchy_depend IN LISTS _vtk_serdes_hierarchy_depends)
    _vtk_module_get_module_property("${_vtk_serdes_hierarchy_depend}"
      PROPERTY  "hierarchy"
      VARIABLE  _vtk_serdes_hierarchy_file)
    if (_vtk_serdes_hierarchy_file)
      list(APPEND _vtk_serdes_hierarchy_files "${_vtk_serdes_hierarchy_file}")
      get_property(_vtk_serdes_is_imported
        TARGET   "${_vtk_serdes_hierarchy_depend}"
        PROPERTY "IMPORTED")
      if (_vtk_serdes_is_imported OR CMAKE_GENERATOR MATCHES "Ninja")
        list(APPEND _vtk_serdes_command_depends "${_vtk_serdes_hierarchy_file}")
      else ()
        _vtk_module_get_module_property("${_vtk_serdes_hierarchy_depend}"
          PROPERTY "library_name"
          VARIABLE _vtk_serdes_hierarchy_library_name)
        if (TARGET "${_vtk_serdes_hierarchy_library_name}-hierarchy")
          list(APPEND _vtk_serdes_command_depends "${_vtk_serdes_hierarchy_library_name}-hierarchy")
        else ()
          message(FATAL_ERROR
            "The ${_vtk_serdes_hierarchy_depend} hierarchy file is attached to a non-imported target "
            "and a hierarchy target (${_vtk_serdes_hierarchy_library_name}-hierarchy) is "
            "missing.")
        endif ()
      endif ()
    endif ()
  endforeach ()

  set(_vtk_serdes_genex_allowed 1)
  _vtk_module_real_target(_vtk_serdes_real_target_name "${_vtk_serdes_MODULE}")
  if (CMAKE_VERSION VERSION_LESS "3.19")
    get_property(_vtk_serdes_target_type
      TARGET  "${_vtk_serdes_real_target_name}"
      PROPERTY TYPE)
    if (_vtk_serdes_target_type STREQUAL "INTERFACE_LIBRARY")
      set(_vtk_serdes_genex_allowed 0)
    endif ()
  endif ()

  set(_vtk_serdes_genex_compile_definitions "")
  set(_vtk_serdes_genex_include_directories "")
  if (_vtk_serdes_genex_allowed)
    set(_vtk_serdes_genex_compile_definitions
      "$<TARGET_PROPERTY:${_vtk_serdes_real_target_name},COMPILE_DEFINITIONS>")
    set(_vtk_serdes_genex_include_directories
      "$<TARGET_PROPERTY:${_vtk_serdes_real_target_name},INCLUDE_DIRECTORIES>")
  else ()
    if (NOT DEFINED ENV{CI})
      message(AUTHOR_WARNING
        "The generation of serialization sources is not using target-local compile definitions or "
        "include directories. This may affect generation of the serialization "
        "sources for the ${_vtk_serdes_MODULE} module. Use CMake 3.19+ to "
        "guarantee intended behavior.")
    endif ()
  endif ()
  file(GENERATE
    OUTPUT  "${_vtk_serdes_args_file}"
    CONTENT "$<$<BOOL:${_vtk_serdes_genex_compile_definitions}>:\n-D\'$<JOIN:${_vtk_serdes_genex_compile_definitions},\'\n-D\'>\'>\n
$<$<BOOL:${_vtk_serdes_genex_include_directories}>:\n-I\'$<JOIN:${_vtk_serdes_genex_include_directories},\'\n-I\'>\'>\n
$<$<BOOL:${_vtk_serdes_hierarchy_files}>:\n--types \'$<JOIN:${_vtk_serdes_hierarchy_files},\'\n--types \'>\'>\n")

  set(_vtk_serdes_classes)
  set(_vtk_serdes_sources)

  # Get the list of public headers from the module
  _vtk_module_get_module_property("${_vtk_serdes_MODULE}"
    PROPERTY  "headers"
    VARIABLE  _vtk_serdes_headers)

  foreach (_vtk_serdes_header IN LISTS _vtk_serdes_headers)
    # Assume the class name matches the basename of the header file. This is a VTK convention
    get_filename_component(_vtk_serdes_classname "${_vtk_serdes_header}" NAME_WE)
    set(_vtk_serdes_source_output
      "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_serdes_classname}SerDes.cxx")
    set(_vtk_serdes_depfile_genex
      "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_serdes_classname}SerDes.cxx.$<CONFIG>.d")
    set(_vtk_serdes_depfile_nogenex
      "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_serdes_classname}SerDes.cxx.d")
    list(APPEND _vtk_serdes_classes
      "${_vtk_serdes_classname}")
    list(APPEND _vtk_serdes_sources
      ${_vtk_serdes_source_output})

    set(_vtk_wrap_serdes_target "VTK::WrapSerDes")
    set(_vtk_serdes_macros_args)
    if (TARGET VTKCompileTools::WrapSerDes)
      set(_vtk_wrap_serdes_target "VTKCompileTools::WrapSerDes")
      if (TARGET VTKCompileTools_macros)
        list(APPEND _vtk_serdes_command_depends
          "VTKCompileTools_macros")
        list(APPEND _vtk_serdes_macros_args
          -undef
          -imacros "${_VTKCompileTools_macros_file}")
      endif ()
    endif ()

    _vtk_module_depfile_args(
      MULTI_CONFIG_NEEDS_GENEX
      TOOL_ARGS _vtk_serdes_depfile_flags
      CUSTOM_COMMAND_ARGS _vtk_serdes_depfile_args
      SOURCE "${_vtk_serdes_header}"
      DEPFILE_PATH "${_vtk_serdes_depfile_genex}"
      DEPFILE_NO_GENEX_PATH "${_vtk_serdes_depfile_nogenex}"
      TOOL_FLAGS "-MF")

    add_custom_command(
      OUTPUT  "${_vtk_serdes_source_output}"
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
              "$<TARGET_FILE:${_vtk_wrap_serdes_target}>"
              ${_vtk_serdes_depfile_flags}
              "@${_vtk_serdes_args_file}"
              -o "${_vtk_serdes_source_output}"
              "${_vtk_serdes_header}"
              ${_vtk_serdes_macros_args}
      ${_vtk_serdes_depfile_args}
      COMMENT "Generating serialization sources for ${_vtk_serdes_classname}"
      DEPENDS
        "${_vtk_serdes_header}"
        "${_vtk_serdes_args_file}"
        "$<TARGET_FILE:${_vtk_wrap_serdes_target}>"
        ${_vtk_serdes_command_depends})
  endforeach ()

  set("${_vtk_serdes_SERIALIZED_CLASSES}"
    "${_vtk_serdes_classes}"
    PARENT_SCOPE)
  set("${_vtk_serdes_SERDES_SOURCES}"
    "${_vtk_serdes_sources}"
    PARENT_SCOPE)
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_serdes

  Generate (de)serialization code for all classes in a ``MODULE``. |module-serialization|

  .. code-block:: cmake

    vtk_module_serdes(
      MODULE            <module>
      EXPORT_MACRO_NAME <export_macro_name>
      EXPORT_FILE_NAME  <export_file_name>
      REGISTRAR_HEADER  <header>
      REGISTRAR_SOURCE  <source>
      SERDES_SOURCES    <serdes_sources>)

  * ``MODULE``: The name of a module.
  * ``EXPORT_MACRO_NAME``: The name of the export macro for ``MODULE``.
  * ``EXPORT_FILE_NAME``: The name of the header file which defines ``EXPORT_MACRO_NAME``
    for ``MODULE``.
  * ``REGISTRAR_HEADER``: This file will contain declaration of registrar function for ``MODULE``.
  * ``REGISTRAR_SOURCE``: This file will contain implementation of registrar function for ``MODULE``.
  * ``SERDES_SOURCES``: This variable holds the list of generated source files with vtkWrapSerDes
    for all classes in ``MODULE``.

#]==]
function (vtk_module_serdes)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_serdes
    ""
    "MODULE;EXPORT_MACRO_NAME;EXPORT_FILE_NAME;REGISTRAR_HEADER;REGISTRAR_SOURCE;SERDES_SOURCES"
    "")

  if (_vtk_serdes_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_serdes: "
      "${_vtk_serdes_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_serdes_MODULE)
    message(FATAL_ERROR "No module was specified!")
  endif ()

  if (NOT _vtk_serdes_EXPORT_MACRO_NAME)
    message(FATAL_ERROR "No export macro name was specified!")
  endif ()

  if (NOT _vtk_serdes_EXPORT_FILE_NAME)
    message(FATAL_ERROR "No export file name was specified!")
  endif ()

  if (NOT _vtk_serdes_REGISTRAR_HEADER)
    message(FATAL_ERROR "No registrar header file name was specified!")
  endif ()

  if (NOT _vtk_serdes_REGISTRAR_SOURCE)
    message(FATAL_ERROR "No registrar source file name was specified!")
  endif ()

  if (NOT _vtk_serdes_SERDES_SOURCES)
    message(FATAL_ERROR "No serdes sources variable name was specified!")
  endif ()

  _vtk_module_get_module_property("${_vtk_serdes_MODULE}"
    PROPERTY  "exclude_wrap"
    VARIABLE  _vtk_serdes_exclude_wrap)
  if (_vtk_serdes_exclude_wrap)
    return ()
  endif ()

  _vtk_module_get_module_property("${_vtk_serdes_MODULE}"
    PROPERTY  "include_marshal"
    VARIABLE  _vtk_serdes_include_marshal)
  if (NOT _vtk_serdes_include_marshal)
    return ()
  endif ()

  set(_vtk_serdes_sources)
  _vtk_module_serdes_generate_sources(
    MODULE                "${_vtk_serdes_MODULE}"
    SERIALIZED_CLASSES    _vtk_serdes_classes
    SERDES_SOURCES        _vtk_serdes_sources)

  if (NOT _vtk_serdes_sources)
    return ()
  endif ()

  vtk_module_generate_library_serdes_registrar(
    MODULE             ${_vtk_serdes_MODULE}
    EXPORT_MACRO_NAME  "${_vtk_serdes_EXPORT_MACRO_NAME}"
    EXPORT_FILE_NAME   "${_vtk_serdes_EXPORT_FILE_NAME}"
    REGISTRAR_HEADER   "${_vtk_serdes_REGISTRAR_HEADER}"
    REGISTRAR_SOURCE   "${_vtk_serdes_REGISTRAR_SOURCE}"
    CLASSES            ${_vtk_serdes_classes})

  set("${_vtk_serdes_SERDES_SOURCES}"
    "${_vtk_serdes_sources}"
    PARENT_SCOPE)
endfunction()


#[==[.rst:
.. cmake:command:: vtk_module_add_serdes_wasm_package

  Adds a WebAssembly package that facilitates deserialization of VTK classes
  in a WASM environment for ``MODULE``. |module-wrapping-serdes|

  .. code-block:: cmake

    vtk_module_add_serdes_wasm_package(
      [ASYNC]
      MODULE                <module>
      OUTPUT_NAME           <output_name>
      [INSTALL_COMPONENT     <install_component>]
      [INSTALL_DESTINATION   <install_destination>]
      [EXTRA_SOURCES         <extra_sources>...]
      [EXTRA_COMPILE_OPTIONS <extra_compile_options>...]
      [EXTRA_LINK_OPTIONS    <extra_link_options>...])

  * ``ASYNC``: If specified, the generated package will be linked with
    `-sJSPI=1` flag. The generated package will support asynchronous
    string-based method calls. This option is required if ``MODULE``
    depends on ``VTK::RenderingWebGPU`` to support asynchronous webgpu APIs.
  * ``MODULE``: The name of a module.
  * ``OUTPUT_NAME``: The name of the output file that will be generated.
    This file will contain the bindings for the WebAssembly module.
  * ``INSTALL_COMPONENT``: Installation component of the install rules created by this function.
    Defaults to ``runtime``.
  * ``INSTALL_DESTINATION``: The destination directory where the generated
    WebAssembly package will be installed. Defaults to ``${CMAKE_INSTALL_BINDIR}``.
  * ``EXTRA_SOURCES``: Additional sources that will be compiled into the
    WebAssembly package. This is useful for adding custom JavaScript wrappers
    or other C++ sources that are not part of the VTK module but are needed
    for the WebAssembly package.
  * ``EXTRA_COMPILE_OPTIONS``: Additional compile options that will be
    passed to the WebAssembly package.
  * ``EXTRA_LINK_OPTIONS``: Additional link options that will be
    passed to the WebAssembly package.

#]==]
function (vtk_module_add_serdes_wasm_package)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_serdes
    "ASYNC"
    "MODULE;OUTPUT_NAME"
    "EXTRA_SOURCES;EXTRA_COMPILE_OPTIONS;EXTRA_LINK_OPTIONS")
  if (_vtk_serdes_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_add_serdes_wasm_package: "
      "${_vtk_serdes_UNPARSED_ARGUMENTS}")
  endif ()
  if (NOT _vtk_serdes_MODULE)
    message(FATAL_ERROR "No module name was specified!")
  endif ()
  if (NOT _vtk_serdes_OUTPUT_NAME)
    message(FATAL_ERROR "No output name was specified!")
  endif ()
  if (NOT _vtk_serdes_INSTALL_COMPONENT)
    set(_vtk_serdes_INSTALL_COMPONENT "runtime")
  endif ()
  if (NOT _vtk_serdes_INSTALL_DESTINATION)
    set(_vtk_serdes_INSTALL_DESTINATION "${CMAKE_INSTALL_BINDIR}")
  endif ()

  set(_vtk_serdes_marshal_depends)
  # Get link dependencies
  get_property(_vtk_serdes_public_depends GLOBAL
    PROPERTY "_vtk_module_${_vtk_serdes_MODULE}_depends")
  get_property(_vtk_serdes_private_depends GLOBAL
    PROPERTY "_vtk_module_${_vtk_serdes_MODULE}_private_depends")
  foreach (_vtk_serdes_depend IN LISTS _vtk_serdes_private_depends _vtk_serdes_public_depends)
    # Skip dependencies that do not include marshal support
    _vtk_module_get_module_property("${_vtk_serdes_depend}"
      PROPERTY  "include_marshal"
      VARIABLE  _vtk_serdes_depend_include_marshal)
    if (NOT _vtk_serdes_depend_include_marshal)
      continue ()
    endif ()
    list(APPEND _vtk_serdes_marshal_depends
      "${_vtk_serdes_depend}")
  endforeach ()
  # Get optional dependencies
  get_property(_vtk_serdes_optional_depends GLOBAL
    PROPERTY "_vtk_module_${_vtk_serdes_MODULE}_optional_depends")
  foreach (_vtk_serdes_optional_depend IN LISTS _vtk_serdes_optional_depends)
    # Skip optional dependencies that are not available
    _vtk_module_optional_dependency_exists("${_vtk_serdes_optional_depend}"
      SATISFIED_VAR _vtk_serdes_optional_depend_exists)
    if (NOT _vtk_serdes_optional_depend_exists)
      continue ()
    endif ()
    list(APPEND _vtk_serdes_marshal_depends
      "${_vtk_serdes_optional_depend}")
  endforeach ()

  # Ensure every module that includes marshal support is a dependency
  foreach (_vtk_serdes_module IN LISTS vtk_modules) # Comes from top-level CMakeLists.txt
    if (NOT TARGET "${_vtk_serdes_module}")
      continue ()
    endif ()
    _vtk_module_get_module_property("${_vtk_serdes_module}"
      PROPERTY  "include_marshal"
      VARIABLE  _vtk_serdes_module_include_marshal)
    if (NOT _vtk_serdes_module_include_marshal)
      continue ()
    endif ()
    if (NOT "${_vtk_serdes_module}" IN_LIST _vtk_serdes_marshal_depends
        AND NOT "${_vtk_serdes_module}" STREQUAL "VTK::RenderingWebGPU")
      message(FATAL_ERROR
        "${_vtk_serdes_module} has `INCLUDE_MARSHAL` but it is not a dependency of ${_vtk_serdes_MODULE}")
    endif ()
  endforeach ()

  # Generate code that registers (de)serialization functions for all classes
  # in all the dependencies.
  set(_vtk_serdes_include_libraries_registrar_headers "")
  set(_vtk_serdes_register_libraries "")
  foreach (_vtk_serdes_dependency IN LISTS _vtk_serdes_marshal_depends)
    # Skip dependencies that do not include marshal support
    _vtk_module_get_module_property("${_vtk_serdes_dependency}"
      PROPERTY  "include_marshal"
      VARIABLE  _vtk_serdes_dependency_include_marshal)
    if (NOT _vtk_serdes_dependency_include_marshal)
      continue ()
    endif ()
    _vtk_module_get_module_property("${_vtk_serdes_dependency}"
      PROPERTY "library_name"
      VARIABLE _vtk_serdes_library_name)
    string(APPEND _vtk_serdes_include_libraries_registrar_headers
      "#include \"${_vtk_serdes_library_name}SerDes.h\"\n")
    string(APPEND _vtk_serdes_register_libraries "
if(!RegisterClasses_${_vtk_serdes_library_name}(serializer, deserializer, invoker, error))
{
  return FAIL;
}\n")
  endforeach ()

  configure_file(
    "${_vtkModuleSerialization_dir}/vtkSerializationWebAssemblyBindings.cxx.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_serdes_OUTPUT_NAME}Bindings.cxx"
    @ONLY)

  vtk_module_add_executable(${_vtk_serdes_MODULE}
    "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_serdes_OUTPUT_NAME}Bindings.cxx"
    "${_vtk_serdes_EXTRA_SOURCES}")

  file(CONFIGURE
    OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_serdes_OUTPUT_NAME}.post.js"
    CONTENT "globalThis.createVTKWASM = ${_vtk_serdes_OUTPUT_NAME};\n")
  set(emscripten_link_options)
  list(APPEND emscripten_link_options
    "$<$<CONFIG:Debug>:-gseparate-dwarf>"
    "-lembind"
    "--extern-post-js=${CMAKE_CURRENT_BINARY_DIR}/${_vtk_serdes_OUTPUT_NAME}.post.js"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sALLOW_TABLE_GROWTH=1"
    "-sEXPORT_NAME=${_vtk_serdes_OUTPUT_NAME}"
    "-sENVIRONMENT=node,web"
    "-sEXPORTED_RUNTIME_METHODS=['addFunction','UTF8ToString','FS', 'ENV']")
  if (CMAKE_SIZEOF_VOID_P EQUAL "8")
    list(APPEND emscripten_link_options
      "-sMAXIMUM_MEMORY=16GB")
  else ()
    list(APPEND emscripten_link_options
      "-sMAXIMUM_MEMORY=4GB")
  endif ()
  vtk_module_compile_options(${_vtk_serdes_MODULE}
    PRIVATE
      ${_vtk_serdes_EXTRA_COMPILE_OPTIONS})
  vtk_module_link_options(${_vtk_serdes_MODULE}
    PRIVATE
      ${emscripten_link_options}
      ${_vtk_serdes_EXTRA_LINK_OPTIONS})
  if (_vtk_serdes_ASYNC)
    vtk_module_link_options(${_vtk_serdes_MODULE}
      PRIVATE
        "-Wno-experimental"
        "-sJSPI=1"
        "-sJSPI_EXPORTS=['__wasm_call_ctors']")
  endif ()
  # Ensures unique symbol mangling for test executables to avoid symbol conflicts.
  vtk_add_test_mangling(${_vtk_serdes_MODULE})

  _vtk_module_real_target(_vtk_serdes_real_target_name "${_vtk_serdes_MODULE}")
  vtk_module_autoinit(
    TARGETS ${_vtk_serdes_real_target_name}
    MODULES ${_vtk_serdes_marshal_depends})
  # The SUFFIX property is set to ".mjs" to ensure the output JavaScript module has the correct extension for ES modules.
  set_target_properties(${_vtk_serdes_real_target_name}
    PROPERTIES
      OUTPUT_NAME "${_vtk_serdes_OUTPUT_NAME}"
      SUFFIX ".mjs")
  # [cmake/cmake#20745](https://gitlab.kitware.com/cmake/cmake/-/issues/20745)
  # CMake doesn't install multiple files associated with an executable target.
  install(FILES
    "$<TARGET_FILE_DIR:${_vtk_serdes_real_target_name}>/${_vtk_serdes_OUTPUT_NAME}.wasm"
    COMPONENT ${_vtk_serdes_INSTALL_COMPONENT}
    DESTINATION ${_vtk_serdes_INSTALL_DESTINATION})
endfunction ()
