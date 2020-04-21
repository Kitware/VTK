#[==[
@defgroup module-wrapping-python Module Python CMake APIs
#]==]

#[==[
@file vtkModuleWrapPython.cmake
@brief APIs for wrapping modules for Python

@section Limitations

Known limitations include:

  - Shared Python modules only really support shared builds of modules. VTK
    does not provide mangling facilities for itself, so statically linking VTK
    into its Python modules precludes using VTK's C++ interface anywhere else
    within the Python environment.
  - Only supports CPython. Other implementations are not supported by the
    `VTK::WrapPython` executable.
  - Links directly to a Python library. See the `VTK::Python` module for more
    details.
#]==]

#[==[
@ingroup module-wrapping-python
@brief Determine Python module destination

Some projects may need to know where Python expects its modules to be placed in
the install tree (assuming a shared prefix). This function computes the default
and sets the passed variable to the value in the calling scope.

~~~
vtk_module_python_default_destination(<var>
  [MAJOR_VERSION <major>])
~~~

By default, the destination is `${CMAKE_INSTALL_BINDIR}/Lib/site-packages` on
Windows and `${CMAKE_INSTALL_LIBDIR}/python<VERSION>/site-packages` otherwise.

`<MAJOR_VERSION>` must be one of `2` or `3`. If not specified, it defaults to
the value of `${VTK_PYTHON_VERSION}`.
#]==]
function (vtk_module_python_default_destination var)
  cmake_parse_arguments(_vtk_module_python
    ""
    "MAJOR_VERSION"
    ""
    ${ARGN})

  if (_vtk_module_python_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_python_default_destination: "
      "${_vtk_module_python_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_module_python_MAJOR_VERSION)
    if (NOT DEFINED VTK_PYTHON_VERSION)
      message(FATAL_ERROR
        "A major version of Python must be specified (or `VTK_PYTHON_VERSION` "
        "be set).")
    endif ()

    set(_vtk_module_python_MAJOR_VERSION "${VTK_PYTHON_VERSION}")
  endif ()

  if (NOT _vtk_module_python_MAJOR_VERSION STREQUAL "2" AND
      NOT _vtk_module_python_MAJOR_VERSION STREQUAL "3")
    message(FATAL_ERROR
      "Only Python2 and Python3 are supported right now.")
  endif ()

  if (WIN32 AND NOT CYGWIN)
    set(destination "${CMAKE_INSTALL_BINDIR}/Lib/site-packages")
  else ()
    if (NOT DEFINED "Python${_vtk_module_python_MAJOR_VERSION}_VERSION_MAJOR" OR
        NOT DEFINED "Python${_vtk_module_python_MAJOR_VERSION}_VERSION_MINOR")
      find_package("Python${_vtk_module_python_MAJOR_VERSION}" QUIET COMPONENTS Development.Module)
    endif ()

    if (Python${_vtk_module_python_MAJOR_VERSION}_VERSION_MAJOR AND Python${_vtk_module_python_MAJOR_VERSION}_VERSION_MINOR)
      set(_vtk_python_version_suffix "${Python${VTK_PYTHON_VERSION}_VERSION_MAJOR}.${Python${VTK_PYTHON_VERSION}_VERSION_MINOR}")
    else ()
      message(WARNING
        "The version of Python is unknown; not using a versioned directory "
        "for Python modules.")
      set(_vtk_python_version_suffix)
    endif ()
    set(destination "${CMAKE_INSTALL_LIBDIR}/python${_vtk_python_version_suffix}/site-packages")
  endif ()

  set("${var}" "${destination}" PARENT_SCOPE)
endfunction ()

#[==[
@ingroup module-impl
@brief Generate sources for using a module's classes from Python

This function generates the wrapped sources for a module. It places the list of
generated source files and classes in variables named in the second and third
arguments, respectively.

~~~
_vtk_module_wrap_python_sources(<module> <sources> <classes>)
~~~
#]==]
function (_vtk_module_wrap_python_sources module sources classes)
  _vtk_module_get_module_property("${module}"
    PROPERTY  "exclude_wrap"
    VARIABLE  _vtk_python_exclude_wrap)
  if (_vtk_python_exclude_wrap)
    return ()
  endif ()

  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_python_library_name}Python")

  set(_vtk_python_args_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_python_library_name}Python/${_vtk_python_library_name}-python.$<CONFIGURATION>.args")

  set(_vtk_python_hierarchy_depends "${module}")
  _vtk_module_get_module_property("${module}"
    PROPERTY  "private_depends"
    VARIABLE  _vtk_python_private_depends)
  list(APPEND _vtk_python_hierarchy_depends ${_vtk_python_private_depends})
  
  set(_vtk_python_command_depends)
  foreach (_vtk_python_hierarchy_depend IN LISTS _vtk_python_hierarchy_depends)
    _vtk_module_get_module_property("${_vtk_python_hierarchy_depend}"
      PROPERTY  "hierarchy"
      VARIABLE  _vtk_python_hierarchy_file)
    if (_vtk_python_hierarchy_file)
      list(APPEND _vtk_python_hierarchy_files "${_vtk_python_hierarchy_file}")
      get_property(_vtk_python_is_imported
        TARGET    "${_vtk_python_hierarchy_depend}"
        PROPERTY  "IMPORTED")
      if (_vtk_python_is_imported OR CMAKE_GENERATOR MATCHES "Ninja")
        list(APPEND _vtk_python_command_depends "${_vtk_python_hierarchy_file}")
      else ()
        _vtk_module_get_module_property("${_vtk_python_hierarchy_depend}"
          PROPERTY  "library_name"
          VARIABLE  _vtk_python_hierarchy_library_name)
        if (TARGET "${_vtk_python_hierarchy_library_name}-hierarchy")
          list(APPEND _vtk_python_command_depends "${_vtk_python_hierarchy_library_name}-hierarchy")
        else ()
          message(FATAL_ERROR
            "The ${_vtk_python_hierarchy_depend} hierarchy file is attached to a non-imported target "
            "and a hierarchy target (${_vtk_python_hierarchy_library_name}-hierarchy) is "
            "missing.")
        endif ()
      endif ()
    endif ()
  endforeach ()

  set(_vtk_python_genex_compile_definitions
    "$<TARGET_PROPERTY:${_vtk_python_target_name},COMPILE_DEFINITIONS>")
  set(_vtk_python_genex_include_directories
    "$<TARGET_PROPERTY:${_vtk_python_target_name},INCLUDE_DIRECTORIES>")
  file(GENERATE
    OUTPUT  "${_vtk_python_args_file}"
    CONTENT "$<$<BOOL:${_vtk_python_genex_compile_definitions}>:\n-D\'$<JOIN:${_vtk_python_genex_compile_definitions},\'\n-D\'>\'>\n
$<$<BOOL:${_vtk_python_genex_include_directories}>:\n-I\'$<JOIN:${_vtk_python_genex_include_directories},\'\n-I\'>\'>\n
$<$<BOOL:${_vtk_python_hierarchy_files}>:\n--types \'$<JOIN:${_vtk_python_hierarchy_files},\'\n--types \'>\'>\n")

  set(_vtk_python_sources)

  # Get the list of public headers from the module.
  _vtk_module_get_module_property("${module}"
    PROPERTY  "headers"
    VARIABLE  _vtk_python_headers)
  set(_vtk_python_classes)
  foreach (_vtk_python_header IN LISTS _vtk_python_headers)
    # Assume the class name matches the basename of the header. This is VTK
    # convention.
    get_filename_component(_vtk_python_basename "${_vtk_python_header}" NAME_WE)
    list(APPEND _vtk_python_classes
      "${_vtk_python_basename}")

    set(_vtk_python_source_output
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_python_library_name}Python/${_vtk_python_basename}Python.cxx")
    list(APPEND _vtk_python_sources
      "${_vtk_python_source_output}")

    set(_vtk_python_wrap_target "VTK::WrapPython")
    set(_vtk_python_macros_args)
    if (TARGET VTKCompileTools::WrapPython)
      set(_vtk_python_wrap_target "VTKCompileTools::WrapPython")
      if (TARGET VTKCompileTools_macros)
        list(APPEND _vtk_python_command_depends
          "VTKCompileTools_macros")
        list(APPEND _vtk_python_macros_args
          -undef
          -imacros "${_VTKCompileTools_macros_file}")
      endif ()
    endif ()

    add_custom_command(
      OUTPUT  "${_vtk_python_source_output}"
      COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
              "$<TARGET_FILE:${_vtk_python_wrap_target}>"
              "@${_vtk_python_args_file}"
              -o "${_vtk_python_source_output}"
              "${_vtk_python_header}"
              ${_vtk_python_macros_args}
      IMPLICIT_DEPENDS
              CXX "${_vtk_python_header}"
      COMMENT "Generating Python wrapper sources for ${_vtk_python_basename}"
      DEPENDS
        "${_vtk_python_header}"
        "${_vtk_python_args_file}"
        "$<TARGET_FILE:${_vtk_python_wrap_target}>"
        ${_vtk_python_command_depends})
  endforeach ()

  set("${sources}"
    "${_vtk_python_sources}"
    PARENT_SCOPE)
  set("${classes}"
    "${_vtk_python_classes}"
    PARENT_SCOPE)
endfunction ()

#[==[
@ingroup module-impl
@brief Generate a CPython library for a set of modules

A Python module library may consist of the Python wrappings of multiple
modules. This is useful for kit-based builds where the modules part of the same
kit belong to the same Python module as well.

~~~
_vtk_module_wrap_python_library(<name> <module>...)
~~~

The first argument is the name of the Python module. The remaining arguments
are modules to include in the Python module.

The remaining information it uses is assumed to be provided by the
@ref vtk_module_wrap_python function.
#]==]
function (_vtk_module_wrap_python_library name)
  set(_vtk_python_library_sources)
  set(_vtk_python_library_classes)
  foreach (_vtk_python_module IN LISTS ARGN)
    _vtk_module_get_module_property("${_vtk_python_module}"
      PROPERTY  "exclude_wrap"
      VARIABLE  _vtk_python_exclude_wrap)
    if (_vtk_python_exclude_wrap)
      continue ()
    endif ()
    _vtk_module_real_target(_vtk_python_target_name "${_vtk_python_module}")
    _vtk_module_get_module_property("${_vtk_python_module}"
      PROPERTY  "library_name"
      VARIABLE  _vtk_python_library_name)

    # Wrap the module independently of the other VTK modules in the Python
    # module.
    _vtk_module_wrap_python_sources("${_vtk_python_module}" _vtk_python_sources _vtk_python_classes)
    list(APPEND _vtk_python_library_sources
      ${_vtk_python_sources})
    list(APPEND _vtk_python_library_classes
      ${_vtk_python_classes})

    # Make sure the module doesn't already have an associated Python package.
    vtk_module_get_property("${_vtk_python_module}"
      PROPERTY  "INTERFACE_vtk_module_python_package"
      VARIABLE  _vtk_python_current_python_package)
    if (DEFINED _vtk_python_current_python_package)
      message(FATAL_ERROR
        "It appears as though the ${_vtk_python_module} has already been "
        "wrapped in Python in the ${_vtk_python_current_python_package} "
        "package.")
    endif ()
    vtk_module_set_property("${_vtk_python_module}"
      PROPERTY  "INTERFACE_vtk_module_python_package"
      VALUE     "${_vtk_python_PYTHON_PACKAGE}")

    if (_vtk_python_INSTALL_HEADERS)
      _vtk_module_export_properties(
        BUILD_FILE    "${_vtk_python_properties_build_file}"
        INSTALL_FILE  "${_vtk_python_properties_install_file}"
        MODULE        "${_vtk_python_module}"
        PROPERTIES
          # Export the wrapping hints file.
          INTERFACE_vtk_module_python_package)
    endif ()
  endforeach ()

  # The foreach needs to be split so that dependencies are guaranteed to have
  # the INTERFACE_vtk_module_python_package property set.
  foreach (_vtk_python_module IN LISTS ARGN)
    _vtk_module_get_module_property("${_vtk_python_module}"
      PROPERTY  "exclude_wrap"
      VARIABLE  _vtk_python_exclude_wrap)
    if (_vtk_python_exclude_wrap)
      continue ()
    endif ()

    _vtk_module_get_module_property("${_vtk_python_module}"
      PROPERTY  "library_name"
      VARIABLE  _vtk_python_library_name)

    _vtk_module_get_module_property("${_vtk_python_module}"
      PROPERTY  "depends"
      VARIABLE  _vtk_python_module_depends)
    set(_vtk_python_module_load_depends)
    foreach (_vtk_python_module_depend IN LISTS _vtk_python_module_depends)
      _vtk_module_get_module_property("${_vtk_python_module_depend}"
        PROPERTY  "exclude_wrap"
        VARIABLE  _vtk_python_module_depend_exclude_wrap)
      if (_vtk_python_module_depend_exclude_wrap)
        continue ()
      endif ()

      _vtk_module_get_module_property("${_vtk_python_module_depend}"
        PROPERTY  "python_package"
        VARIABLE  _vtk_python_depend_module_package)
      _vtk_module_get_module_property("${_vtk_python_module_depend}"
        PROPERTY  "library_name"
        VARIABLE  _vtk_python_depend_library_name)

      # XXX(kits): This doesn't work for kits.
      list(APPEND _vtk_python_module_load_depends
        "${_vtk_python_depend_module_package}.${_vtk_python_depend_library_name}")
    endforeach ()

    if (_vtk_python_BUILD_STATIC)
      # If static, we use .py modules that grab the contents from the baked-in modules.
      set(_vtk_python_module_file
        "${CMAKE_BINARY_DIR}/${_vtk_python_MODULE_DESTINATION}/${_vtk_python_package_path}/${_vtk_python_library_name}.py")
      set(_vtk_python_module_contents
          "from ${_vtk_python_import_prefix}${_vtk_python_library_name} import *\n")

      file(GENERATE
        OUTPUT  "${_vtk_python_module_file}"
        CONTENT "${_vtk_python_module_contents}")

      # Set `python_modules` to provide the list of python files that go along with
      # this module
      _vtk_module_set_module_property("${_vtk_python_module}" APPEND
        PROPERTY  "python_modules"
        VALUE     "${_vtk_python_module_file}")
    endif ()
  endforeach ()

  if (NOT _vtk_python_library_sources)
    return ()
  endif ()

  set(_vtk_python_init_data_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${name}Python/${name}-init.data")

  file(GENERATE
    OUTPUT  "${_vtk_python_init_data_file}"
    CONTENT "${_vtk_python_library_name}\n$<JOIN:${_vtk_python_classes},\n>\nDEPENDS\n$<JOIN:${_vtk_python_module_load_depends},\n>\n")

  set(_vtk_python_init_output
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${name}Python/${name}Init.cxx")
  set(_vtk_python_init_impl_output
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${name}Python/${name}InitImpl.cxx")
  list(APPEND _vtk_python_library_sources
    "${_vtk_python_init_output}"
    "${_vtk_python_init_impl_output}")

  set(_vtk_python_wrap_target "VTK::WrapPythonInit")
  if (TARGET VTKCompileTools::WrapPythonInit)
    set(_vtk_python_wrap_target "VTKCompileTools::WrapPythonInit")
  endif ()

  if(_vtk_python_BUILD_STATIC)
    set(additonal_options "${_vtk_python_import_prefix}")
  endif()
  add_custom_command(
    OUTPUT  "${_vtk_python_init_output}"
            "${_vtk_python_init_impl_output}"
    COMMAND "${_vtk_python_wrap_target}"
            "${_vtk_python_init_data_file}"
            "${_vtk_python_init_output}"
            "${_vtk_python_init_impl_output}"
            "${additonal_options}"
    COMMENT "Generating the Python module initialization sources for ${name}"
    DEPENDS
      "${_vtk_python_init_data_file}"
      "$<TARGET_FILE:${_vtk_python_wrap_target}>")

  if (_vtk_python_BUILD_STATIC)
    set(_vtk_python_module_header_file
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${name}/static_python/${name}.h")
    set(_vtk_python_module_header_content
"#ifndef ${name}_h
#define ${name}_h

#include <vtkPython.h>

#ifdef __cplusplus
extern \"C\" {
#endif
#if PY_VERSION_HEX < 0x03000000
extern void init${_vtk_python_library_name}();
#else
extern PyObject* PyInit_${_vtk_python_library_name}();
#endif
#ifdef __cplusplus
}
#endif

#endif
")

    file(GENERATE
      OUTPUT  "${_vtk_python_module_header_file}"
      CONTENT "${_vtk_python_module_header_content}")
    # XXX(cmake): Why is this necessary? One would expect that `file(GENERATE)`
    # would do this automatically.
    set_property(SOURCE "${_vtk_python_module_header_file}"
      PROPERTY
        GENERATED 1)

    add_library("${name}" STATIC
      ${_vtk_python_library_sources}
      "${_vtk_python_module_header_file}")
    target_include_directories("${name}"
      INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${name}/static_python>")
    target_link_libraries("${name}"
      PUBLIC
        VTK::Python)
    set_property(TARGET "${name}"
      PROPERTY
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_vtk_python_STATIC_MODULE_DESTINATION}")
  else ()
    add_library("${name}" MODULE
      ${_vtk_python_library_sources})
    if (WIN32 AND NOT CYGWIN)
      # XXX(python-debug): This is disabled out because there's no reliable way
      # to tell whether we're using a debug build of Python or not. Since using
      # a debug Python build is so rare, just assume we're always using a
      # non-debug build of Python itself.
      #
      # The proper fix is to dig around and ask the backing `PythonN::Python`
      # target used by `VTK::Python` for its properties to find out, per
      # configuration, whether it is a debug build. If it is, add the postfix
      # (regardless of VTK's build type). Otherwise, no postfix.
      if (FALSE)
        set_property(TARGET "${name}"
          APPEND_STRING
          PROPERTY
            DEBUG_POSTFIX "_d")
      endif ()
      set_property(TARGET "${name}"
        PROPERTY
          SUFFIX ".pyd")
    endif ()
    set_property(TARGET "${name}"
      PROPERTY
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_vtk_python_MODULE_DESTINATION}/${_vtk_python_package_path}")
    get_property(_vtk_python_is_multi_config GLOBAL
      PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if (_vtk_python_is_multi_config)
      # XXX(MultiNinja): This isn't going to work in general since MultiNinja
      # will error about overlapping output paths.
      foreach (_vtk_python_config IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${_vtk_python_config}" _vtk_python_config_upper)
        set_property(TARGET "${name}"
          PROPERTY
            "LIBRARY_OUTPUT_DIRECTORY_${_vtk_python_config_upper}" "${CMAKE_BINARY_DIR}/${_vtk_python_MODULE_DESTINATION}/${_vtk_python_package_path}")
      endforeach ()
    endif ()
    set_target_properties("${name}"
      PROPERTIES
        PREFIX ""
        OUTPUT_NAME "${_vtk_python_library_name}"
        ARCHIVE_OUTPUT_NAME "${name}")
  endif ()

  vtk_module_autoinit(
    MODULES ${ARGN}
    TARGETS "${name}")

  # The wrapper code will expand PYTHON_PACKAGE as needed
  target_compile_definitions("${name}"
    PRIVATE
      "-DPYTHON_PACKAGE=\"${_vtk_python_PYTHON_PACKAGE}\"")

  target_link_libraries("${name}"
    PRIVATE
      ${ARGN}
      VTK::WrappingPythonCore
      VTK::Python)

  set(_vtk_python_export)
  if (_vtk_python_INSTALL_EXPORT)
    set(_vtk_python_export
      EXPORT "${_vtk_python_INSTALL_EXPORT}")
  endif ()

  install(
    TARGETS             "${name}"
    ${_vtk_python_export}
    COMPONENT           "${_vtk_python_COMPONENT}"
    RUNTIME DESTINATION "${_vtk_python_MODULE_DESTINATION}/${_vtk_python_package_path}"
    LIBRARY DESTINATION "${_vtk_python_MODULE_DESTINATION}/${_vtk_python_package_path}"
    ARCHIVE DESTINATION "${_vtk_python_STATIC_MODULE_DESTINATION}")
endfunction ()

#[==[
@ingroup module-wrapping-python
@brief Wrap a set of modules for use in Python

~~~
vtk_module_wrap_python(
  MODULES <module>...
  [TARGET <target>]
  [WRAPPED_MODULES <varname>]

  [BUILD_STATIC <ON|OFF>]
  [INSTALL_HEADERS <ON|OFF>]

  [DEPENDS <target>...]

  [MODULE_DESTINATION <destination>]
  [STATIC_MODULE_DESTINATION <destination>]
  [CMAKE_DESTINATION <destination>]
  [LIBRARY_DESTINATION <destination>]

  [PYTHON_PACKAGE <package>]
  [SOABI <soabi>]

  [INSTALL_EXPORT <export>]
  [COMPONENT <component>])
~~~

  * `MODULES`: (Required) The list of modules to wrap.
  * `TARGET`: (Recommended) The target to create which represents all wrapped
    Python modules. This is mostly useful when supporting static Python modules
    in order to add the generated modules to the built-in table.
  * `WRAPPED_MODULES`: (Recommended) Not all modules are wrappable. This
    variable will be set to contain the list of modules which were wrapped.
    These modules will have a `INTERFACE_vtk_module_python_package` property
    set on them which is the name that should be given to `import` statements
    in Python code.
  * `BUILD_STATIC`: Defaults to `${BUILD_SHARED_LIBS}`. Note that shared
    modules with a static build is not completely supported. For static Python
    module builds, a header named `<TARGET>.h` will be available with a
    function `void <TARGET>_load()` which will add all Python modules created
    by this call to the imported module table. For shared Python module builds,
    the same function is provided, but it is a no-op.
  * `INSTALL_HEADERS` (Defaults to `ON`): If unset, CMake properties will not
    be installed.
  * `DEPENDS`: This is list of other Python modules targets i.e. targets
    generated from previous calls to `vtk_module_wrap_python` that this new
    target depends on. This is used when `BUILD_STATIC` is true to ensure that
    the `void <TARGET>_load()` is correctly called for each of the dependencies.
  * `MODULE_DESTINATION`: Modules will be placed in this location in the
    build tree. The install tree should remove `$<CONFIGURATION>` bits, but it
    currently does not. See `vtk_module_python_default_destination` for the
    default value.
  * `STATIC_MODULE_DESTINATION`: Defaults to `${CMAKE_INSTALL_LIBDIR}`. This
    default may change in the future since the best location for these files is
    not yet known. Static libraries containing Python code will be installed to
    the install tree under this path.
  * `CMAKE_DESTINATION`: (Required if `INSTALL_HEADERS` is `ON`) Where to
    install Python-related module property CMake files.
  * `LIBRARY_DESTINATION` (Recommended): If provided, dynamic loader
    information will be added to modules for loading dependent libraries.
  * `PYTHON_PACKAGE`: (Recommended) All generated modules will be added to this
    Python package. The format is in Python syntax (e.g.,
    `package.subpackage`).
  * `SOABI`: (Required for wheel support): If given, generate libraries with
    the SOABI tag in the module filename.
  * `INSTALL_EXPORT`: If provided, static installs will add the installed
    libraries to the provided export set.
  * `COMPONENT`: Defaults to `python`. All install rules created by this
    function will use this installation component.
#]==]
function (vtk_module_wrap_python)
  cmake_parse_arguments(_vtk_python
    ""
    "MODULE_DESTINATION;STATIC_MODULE_DESTINATION;LIBRARY_DESTINATION;PYTHON_PACKAGE;BUILD_STATIC;INSTALL_HEADERS;INSTALL_EXPORT;TARGET;COMPONENT;WRAPPED_MODULES;CMAKE_DESTINATION;DEPENDS;SOABI"
    "MODULES"
    ${ARGN})

  if (_vtk_python_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_wrap_python: "
      "${_vtk_python_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_python_MODULES)
    message(WARNING
      "No modules were requested for Python wrapping.")
    return ()
  endif ()

  _vtk_module_split_module_name("${_vtk_python_TARGET}" _vtk_python)

  set(_vtk_python_depends)
  foreach (_vtk_python_depend IN LISTS _vtk_python_DEPENDS)
    _vtk_module_split_module_name("${_vtk_python_depend}" _vtk_python_depends)
    list(APPEND _vtk_python_depends
      "${_vtk_python_depends_TARGET_NAME}")
  endforeach ()

  if (NOT DEFINED _vtk_python_MODULE_DESTINATION)
    vtk_module_python_default_destination(_vtk_python_MODULE_DESTINATION)
  endif ()

  if (NOT DEFINED _vtk_python_INSTALL_HEADERS)
    set(_vtk_python_INSTALL_HEADERS ON)
  endif ()

  if (_vtk_python_SOABI)
    get_property(_vtk_python_is_multi_config GLOBAL
      PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if (_vtk_python_is_multi_config)
      foreach (_vtk_python_config IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${_vtk_python_config}" _vtk_python_upper_config)
        set("CMAKE_${_vtk_python_upper_config}_POSTFIX"
          ".${_vtk_python_SOABI}")
      endforeach ()
    else ()
      string(TOUPPER "${CMAKE_BUILD_TYPE}" _vtk_python_upper_config)
      set("CMAKE_${_vtk_python_upper_config}_POSTFIX"
        ".${_vtk_python_SOABI}")
    endif ()
  endif ()

  if (_vtk_python_INSTALL_HEADERS AND NOT DEFINED _vtk_python_CMAKE_DESTINATION)
    message(FATAL_ERROR
      "No CMAKE_DESTINATION set, but headers from the Python wrapping were "
      "requested for install and the CMake files are required to work with "
      "them.")
  endif ()

  if (NOT DEFINED _vtk_python_BUILD_STATIC)
    if (BUILD_SHARED_LIBS)
      set(_vtk_python_BUILD_STATIC OFF)
    else ()
      set(_vtk_python_BUILD_STATIC ON)
    endif ()
  else ()
    if (NOT _vtk_python_BUILD_STATIC AND NOT BUILD_SHARED_LIBS)
      message(WARNING
        "Building shared Python modules against static VTK modules only "
        "supports consuming the VTK modules via their Python interfaces due "
        "to the lack of support for an SDK to use the same static libraries.")
    endif ()
  endif ()

  if (NOT DEFINED _vtk_python_STATIC_MODULE_DESTINATION)
    # TODO: Is this correct?
    set(_vtk_python_STATIC_MODULE_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  endif ()

  if (NOT DEFINED _vtk_python_COMPONENT)
    set(_vtk_python_COMPONENT "python")
  endif ()

  if (NOT _vtk_python_PYTHON_PACKAGE)
    message(FATAL_ERROR
      "No `PYTHON_PACKAGE` was given; Python modules must be placed into a "
      "package.")
  endif ()
  string(REPLACE "." "/" _vtk_python_package_path "${_vtk_python_PYTHON_PACKAGE}")

  if(_vtk_python_BUILD_STATIC)
    # When doing static builds we want the statically initialized built-ins to be
    # used. It is unclear in the Python-C API how to construct `namespace.module`
    # so instead at the C++ level we import "namespace_module" during startup
    # and than the python modules moving those imports into the correct python
    # module.
    string(REPLACE "." "_" _vtk_python_import_prefix "${_vtk_python_PYTHON_PACKAGE}_")
  else()
    # We are building dynamic libraries therefore the prefix is simply '.'
    set(_vtk_python_import_prefix ".")
  endif()

  _vtk_module_check_destinations(_vtk_python_
    MODULE_DESTINATION
    STATIC_MODULE_DESTINATION
    CMAKE_DESTINATION
    LIBRARY_DESTINATION)

  if (_vtk_python_INSTALL_HEADERS)
    set(_vtk_python_properties_filename "${_vtk_python_PYTHON_PACKAGE}-vtk-python-module-properties.cmake")
    set(_vtk_python_properties_install_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_python_TARGET_NAME}/${_vtk_python_properties_filename}.install")
    set(_vtk_python_properties_build_file "${CMAKE_BINARY_DIR}/${_vtk_python_CMAKE_DESTINATION}/${_vtk_python_properties_filename}")

    file(WRITE "${_vtk_python_properties_build_file}")
    file(WRITE "${_vtk_python_properties_install_file}")
  endif ()

  if (DEFINED _vtk_python_LIBRARY_DESTINATION)
    # Set up rpaths
    set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)
    if (UNIX)
      file(RELATIVE_PATH _vtk_python_relpath
        "/prefix/${_vtk_python_MODULE_DESTINATION}/${_vtk_python_package_path}"
        "/prefix/${_vtk_python_LIBRARY_DESTINATION}")

      if (APPLE)
        set(_vtk_python_origin_stem "@loader_path")
      else ()
        set(_vtk_python_origin_stem "$ORIGIN")
      endif()

      list(APPEND CMAKE_INSTALL_RPATH
        "${_vtk_python_origin_stem}/${_vtk_python_relpath}")
    endif ()
  endif ()

  set(_vtk_python_sorted_modules ${_vtk_python_MODULES})
  foreach (_vtk_python_module IN LISTS _vtk_python_MODULES)
    _vtk_module_get_module_property("${_vtk_python_module}"
      PROPERTY  "depends"
      VARIABLE  "_vtk_python_${_vtk_python_module}_depends")
  endforeach ()
  vtk_topological_sort(_vtk_python_sorted_modules "_vtk_python_" "_depends")

  set(_vtk_python_sorted_modules_filtered)
  foreach (_vtk_python_module IN LISTS _vtk_python_sorted_modules)
    if (_vtk_python_module IN_LIST _vtk_python_MODULES)
      list(APPEND _vtk_python_sorted_modules_filtered
        "${_vtk_python_module}")
    endif ()
  endforeach ()

  set(_vtk_python_all_modules)
  set(_vtk_python_all_wrapped_modules)
  foreach (_vtk_python_module IN LISTS _vtk_python_sorted_modules_filtered)
    _vtk_module_get_module_property("${_vtk_python_module}"
      PROPERTY  "library_name"
      VARIABLE  _vtk_python_library_name)
    _vtk_module_wrap_python_library("${_vtk_python_library_name}Python" "${_vtk_python_module}")

    if (TARGET "${_vtk_python_library_name}Python")
      list(APPEND _vtk_python_all_modules
        "${_vtk_python_library_name}Python")
      list(APPEND _vtk_python_all_wrapped_modules
        "${_vtk_python_module}")
    endif ()
  endforeach ()

  if (NOT _vtk_python_all_modules)
    message(FATAL_ERROR
      "No modules given could be wrapped.")
  endif ()

  if (_vtk_python_INSTALL_HEADERS)
    install(
      FILES       "${_vtk_python_properties_install_file}"
      DESTINATION "${_vtk_python_CMAKE_DESTINATION}"
      RENAME      "${_vtk_python_properties_filename}"
      COMPONENT   "development")
  endif ()

  if (DEFINED _vtk_python_WRAPPED_MODULES)
    set("${_vtk_python_WRAPPED_MODULES}"
      "${_vtk_python_all_wrapped_modules}"
      PARENT_SCOPE)
  endif ()

  if (_vtk_python_TARGET)
    add_library("${_vtk_python_TARGET_NAME}" INTERFACE)
    target_include_directories("${_vtk_python_TARGET_NAME}"
      INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_python_TARGET_NAME}/static_python>")
    target_link_libraries("${_vtk_python_TARGET_NAME}"
      INTERFACE
        ${_vtk_python_DEPENDS})
    if (NOT _vtk_python_TARGET STREQUAL _vtk_python_TARGET_NAME)
      add_library("${_vtk_python_TARGET}" ALIAS
        "${_vtk_python_TARGET_NAME}")
    endif ()

    if (_vtk_python_INSTALL_EXPORT)
      install(
        TARGETS   "${_vtk_python_TARGET_NAME}"
        EXPORT    "${_vtk_python_INSTALL_EXPORT}"
        COMPONENT "development")
    endif ()

    set(_vtk_python_all_modules_include_file
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_python_TARGET_NAME}/static_python/${_vtk_python_TARGET_NAME}.h")
    set(_vtk_python_all_modules_include_content
      "#ifndef ${_vtk_python_TARGET_NAME}_h\n#define ${_vtk_python_TARGET_NAME}_h\n")

    if (_vtk_python_BUILD_STATIC)
      foreach (_vtk_python_module IN LISTS _vtk_python_all_modules)
        string(APPEND _vtk_python_all_modules_include_content
          "#include \"${_vtk_python_module}.h\"\n")
      endforeach ()
    endif ()

    foreach (_vtk_python_depend IN LISTS _vtk_python_depends)
      string(APPEND _vtk_python_all_modules_include_content
        "#include \"${_vtk_python_depend}.h\"\n")
    endforeach ()

    string(APPEND _vtk_python_all_modules_include_content
"#if PY_VERSION_HEX < 0x03000000
#define PY_APPEND_INIT(module) PyImport_AppendInittab(\"${_vtk_python_import_prefix}\" #module, init ## module)
#define PY_IMPORT(module) init ## module();
#else
#define PY_APPEND_INIT(module) PyImport_AppendInittab(\"${_vtk_python_import_prefix}\" #module, PyInit_ ## module)
#define PY_IMPORT(module) { \\
    PyObject* var_ ## module = PyInit_ ## module(); \\
    PyDict_SetItemString(PyImport_GetModuleDict(), \"${_vtk_python_import_prefix}\" #module,var_ ## module); \\
    Py_DECREF(var_ ## module); }
#endif

#define PY_APPEND_INIT_OR_IMPORT(module, do_import) \\
  if (do_import) { PY_IMPORT(module); } else { PY_APPEND_INIT(module); }

static void ${_vtk_python_TARGET_NAME}_load() {\n")

    foreach (_vtk_python_depend IN LISTS _vtk_python_depends)
      string(APPEND _vtk_python_all_modules_include_content
        "  ${_vtk_python_depend}_load();\n")
    endforeach ()

    if (_vtk_python_BUILD_STATIC)
      string(APPEND _vtk_python_all_modules_include_content
        "  int do_import = Py_IsInitialized();\n")
      foreach (_vtk_python_module IN LISTS _vtk_python_sorted_modules_filtered)
        _vtk_module_get_module_property("${_vtk_python_module}"
          PROPERTY  "library_name"
          VARIABLE  _vtk_python_library_name)
        if (TARGET "${_vtk_python_library_name}Python")
          string(APPEND _vtk_python_all_modules_include_content
            "  PY_APPEND_INIT_OR_IMPORT(${_vtk_python_library_name}, do_import);\n")
        endif ()
      endforeach ()
    endif ()

    string(APPEND _vtk_python_all_modules_include_content
      "}\n#undef PY_APPEND_INIT\n#undef PY_IMPORT\n#undef PY_APPEND_INIT_OR_IMPORT\n#endif\n")

    # TODO: Install this header.
    file(GENERATE
      OUTPUT  "${_vtk_python_all_modules_include_file}"
      CONTENT "${_vtk_python_all_modules_include_content}")

    if (_vtk_python_BUILD_STATIC)
      # TODO: Install these targets.
      target_link_libraries("${_vtk_python_TARGET_NAME}"
        INTERFACE
          ${_vtk_python_all_modules})
    endif ()

    if (_vtk_python_BUILD_STATIC)
      # Next, we generate a Python module that can be imported to import any
      # static artifacts e.g. all wrapping Python modules in static builds,
      # (eventually, frozen modules etc.)
      string(REPLACE "." "_" _vtk_python_static_importer_name "_${_vtk_python_PYTHON_PACKAGE}_static")
      set(_vtk_python_static_importer_file
        "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_python_TARGET_NAME}/static_python/${_vtk_python_static_importer_name}.c")
      set(_vtk_python_static_importer_content "// generated file, do not edit!
#include <vtkPython.h>
#include \"${_vtk_python_TARGET_NAME}.h\"

  static PyMethodDef Py${_vtk_python_static_importer_name}_Methods[] = {
  {NULL, NULL, 0, NULL}};
#if PY_VERSION_HEX >= 0x03000000
  static PyModuleDef ${_vtk_python_static_importer_name}Module = {
    PyModuleDef_HEAD_INIT,
    \"${_vtk_python_static_importer_name}\", // m_name
    \"module to import static components for ${_vtk_python_TARGET_NAME}\", // m_doc
    0, // m_size
    Py${_vtk_python_static_importer_name}_Methods, // m_methods
    NULL, // m_reload
    NULL, // m_traverse
    NULL, // m_clear
    NULL  // m_free
  };
#endif

#if PY_VERSION_HEX >= 0x03000000
  PyMODINIT_FUNC PyInit_${_vtk_python_static_importer_name}(void)
#else
  PyMODINIT_FUNC init${_vtk_python_static_importer_name}(void)
#endif
  {
    // since this gets called after `Py_Initialize`, this will import the static
    // modules and not just update the init table.
    ${_vtk_python_TARGET_NAME}_load();
#if PY_VERSION_HEX >= 0x03000000
    return PyModule_Create(&${_vtk_python_static_importer_name}Module);
#else
    Py_InitModule(\"${_vtk_python_static_importer_name}\", Py${_vtk_python_static_importer_name}_Methods);
#endif
  }\n")

      # TODO: Install this header.
      file(GENERATE
        OUTPUT  "${_vtk_python_static_importer_file}"
        CONTENT "${_vtk_python_static_importer_content}")

      add_library("${_vtk_python_static_importer_name}" MODULE
        ${_vtk_python_static_importer_file})
      if (WIN32 AND NOT CYGWIN)
        set_property(TARGET "${_vtk_python_static_importer_name}"
          PROPERTY
            SUFFIX ".pyd")
      endif()
      set_property(TARGET "${_vtk_python_static_importer_name}"
        PROPERTY
          LIBRARY_OUTPUT_DIRECTORY "${_vtk_python_MODULE_DESTINATION}")
      get_property(_vtk_python_is_multi_config GLOBAL
        PROPERTY GENERATOR_IS_MULTI_CONFIG)
      if (_vtk_python_is_multi_config)
        # XXX(MultiNinja): This isn't going to work in general since MultiNinja
        # will error about overlapping output paths.
        foreach (_vtk_python_config IN LISTS CMAKE_CONFIGURATION_TYPES)
          string(TOUPPER "${_vtk_python_config}" _vtk_python_config_upper)
          set_property(TARGET "${_vtk_python_static_importer_name}"
            PROPERTY
              "LIBRARY_OUTPUT_DIRECTORY_${_vtk_python_config_upper}" "${CMAKE_BINARY_DIR}/${_vtk_python_MODULE_DESTINATION}")
        endforeach ()
      endif ()
      set_property(TARGET "${_vtk_python_static_importer_name}"
        PROPERTY
          PREFIX "")
      target_link_libraries("${_vtk_python_static_importer_name}"
        PRIVATE
          ${_vtk_python_TARGET_NAME}
          VTK::WrappingPythonCore
          VTK::Python)
      install(
        TARGETS             "${_vtk_python_static_importer_name}"
        COMPONENT           "${_vtk_python_COMPONENT}"
        RUNTIME DESTINATION "${_vtk_python_MODULE_DESTINATION}"
        LIBRARY DESTINATION "${_vtk_python_MODULE_DESTINATION}"
        ARCHIVE DESTINATION "${_vtk_python_STATIC_MODULE_DESTINATION}")
    endif () # if (_vtk_python_BUILD_STATIC)
  endif ()
endfunction ()

#[==[
@ingroup module-wrapping-python
@brief Install Python packages with a module

Some modules may have associated Python code. This function should be used to
install them.

~~~
vtk_module_add_python_package(<module>
  PACKAGE <package>
  FILES <files>...
  [MODULE_DESTINATION <destination>]
  [COMPONENT <component>])
~~~

The `<module>` argument must match the associated VTK module that the package
is with. Each package is independent and should be installed separately. That
is, `package` and `package.subpackage` should each get their own call to this
function.

  * `PACKAGE`: (Required) The package installed by this call. Currently,
    subpackages must have their own call to this function.
  * `FILES`: (Required) File paths should be relative to the source directory
    of the calling `CMakeLists.txt`. Upward paths are not supported (nor are
    checked for). Absolute paths are assumed to be in the build tree and their
    relative path is computed relative to the current binary directory.
  * `MODULE_DESTINATION`: Modules will be placed in this location in the
    build tree. The install tree should remove `$<CONFIGURATION>` bits, but it
    currently does not. See `vtk_module_python_default_destination` for the
    default value.
  * `COMPONENT`: Defaults to `python`. All install rules created by this
    function will use this installation component.

A `<module>-<package>` target is created which ensures that all Python modules
have been copied to the correct location in the build tree.

@todo Support a tree of modules with a single call.

@todo Support freezing the Python package. This should create a header and the
associated target should provide an interface for including this header. The
target should then be exported and the header installed properly.
#]==]
function (vtk_module_add_python_package name)
  if (NOT name STREQUAL _vtk_build_module)
    message(FATAL_ERROR
      "Python modules must match their module names.")
  endif ()

  cmake_parse_arguments(_vtk_add_python_package
    ""
    "PACKAGE;MODULE_DESTINATION;COMPONENT"
    "FILES"
    ${ARGN})

  if (_vtk_add_python_package_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_add_python_package: "
      "${_vtk_add_python_package_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_add_python_package_PACKAGE)
    message(FATAL_ERROR
      "The `PACKAGE` argument is required.")
  endif ()
  string(REPLACE "." "/" _vtk_add_python_package_path "${_vtk_add_python_package_PACKAGE}")

  if (NOT _vtk_add_python_package_FILES)
    message(FATAL_ERROR
      "The `FILES` argument is required.")
  endif ()

  if (NOT DEFINED _vtk_add_python_package_MODULE_DESTINATION)
    vtk_module_python_default_destination(_vtk_add_python_package_MODULE_DESTINATION)
  endif ()

  if (NOT DEFINED _vtk_add_python_package_COMPONENT)
    set(_vtk_add_python_package_COMPONENT "python")
  endif ()

  set(_vtk_add_python_package_file_outputs)
  foreach (_vtk_add_python_package_file IN LISTS _vtk_add_python_package_FILES)
    if (IS_ABSOLUTE "${_vtk_add_python_package_file}")
      file(RELATIVE_PATH _vtk_add_python_package_name
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${_vtk_add_python_package_name}")
    else ()
      set(_vtk_add_python_package_name
        "${_vtk_add_python_package_file}")
      set(_vtk_add_python_package_file
        "${CMAKE_CURRENT_SOURCE_DIR}/${_vtk_add_python_package_file}")
    endif ()

    set(_vtk_add_python_package_file_output
      "${CMAKE_BINARY_DIR}/${_vtk_add_python_package_MODULE_DESTINATION}/${_vtk_add_python_package_name}")
    add_custom_command(
      OUTPUT  "${_vtk_add_python_package_file_output}"
      DEPENDS "${_vtk_add_python_package_file}"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
              "${_vtk_add_python_package_file}"
              "${_vtk_add_python_package_file_output}"
      COMMENT "Copying ${_vtk_add_python_package_name} to the binary directory")
    list(APPEND _vtk_add_python_package_file_outputs
      "${_vtk_add_python_package_file_output}")
    # XXX
    if (BUILD_SHARED_LIBS)
      install(
        FILES       "${_vtk_add_python_package_name}"
        DESTINATION "${_vtk_add_python_package_MODULE_DESTINATION}/${_vtk_add_python_package_path}"
        COMPONENT   "${_vtk_add_python_package_COMPONENT}")
    endif()
  endforeach ()

  get_property(_vtk_add_python_package_module GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_target_name")
  add_custom_target("${_vtk_add_python_package_module}-${_vtk_add_python_package_PACKAGE}" ALL
    DEPENDS
      ${_vtk_add_python_package_file_outputs})

  # Set `python_modules` to provide the list of python files that go along with
  # this module
  set_property(TARGET "${_vtk_add_python_package_module}-${_vtk_add_python_package_PACKAGE}"
    PROPERTY
      "python_modules" "${_vtk_add_python_package_file_outputs}")
endfunction ()

#[==[
@ingroup module-wrapping-python
@brief Use a Python package as a module

If a module is a Python package, this function should be used instead of
@ref vtk_module_add_module.

~~~
vtk_module_add_python_module(<name>
  PACKAGES <packages>...)
~~~

  * `PACKAGES`: (Required) The list of packages installed by this module.
    These must have been created by the @ref vtk_module_add_python_package
    function.
#]==]
function (vtk_module_add_python_module name)
  if (NOT name STREQUAL _vtk_build_module)
    message(FATAL_ERROR
      "Python modules must match their module names.")
  endif ()

  cmake_parse_arguments(_vtk_add_python_module
    ""
    ""
    "PACKAGES"
    ${ARGN})

  if (_vtk_add_python_module_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_add_python_module: "
      "${_vtk_add_python_module_UNPARSED_ARGUMENTS}")
  endif ()

  get_property(_vtk_add_python_module_depends GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_depends")
  get_property(_vtk_add_python_module_target_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_target_name")
  add_library("${_vtk_add_python_module_target_name}" INTERFACE)
  target_link_libraries("${_vtk_add_python_module_target_name}"
    INTERFACE
      ${_vtk_add_python_module_depends})
  if (NOT _vtk_build_module STREQUAL _vtk_add_python_module_target_name)
    add_library("${_vtk_build_module}" ALIAS
      "${_vtk_add_python_module_target_name}")
  endif ()
  foreach (_vtk_add_python_module_package IN LISTS _vtk_add_python_module_PACKAGES)
    add_dependencies("${_vtk_add_python_module_target_name}"
      "${_vtk_build_module}-${_vtk_add_python_module_package}")

    # get the list of python files and add them on the module.
    get_property(_vtk_module_python_modules
      TARGET "${_vtk_add_python_module_target_name}-${_vtk_add_python_module_package}"
      PROPERTY "python_modules")
    _vtk_module_set_module_property("${_vtk_build_module}" APPEND
      PROPERTY  "python_modules"
      VALUE     "${_vtk_module_python_modules}")
  endforeach ()

  _vtk_module_apply_properties("${_vtk_add_python_module_target_name}")
  _vtk_module_install("${_vtk_add_python_module_target_name}")
endfunction ()
