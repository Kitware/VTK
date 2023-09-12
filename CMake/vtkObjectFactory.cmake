#[==[
@file vtkObjectFactory.cmake

In VTK, certain classes can have what are called "implementations". When the
base class is requested, it instead looks into a list of available
implementations. One of the implementations is then constructed and returned
instead.

For instance, there is a `vtkRenderWindow` class in VTK. When this is
constructed, it instead actually returns a window for the X window system,
Cocoa, or Win32 depending on what is available.

VTK's implementation utilizes the
[autoinit](https://docs.vtk.org/en/latest/api/cmake/ModuleSystem.html#autoinit)
logic of the
module system. A module which contains an object factory must declare itself as
`IMPLEMENTABLE` and modules which contain an implementation of an object
factory must claim that they `IMPLEMENTS` modules containing those base object
factories (a module may contain the object factory and an implementation; it
then says that it `IMPLEMENTS` itself).
#]==]

set(_vtkObjectFactory_source_dir "${CMAKE_CURRENT_LIST_DIR}")

#[==[
@brief Declare a factory override

Declare that a class in this module (the implementation) is an `OVERRIDE` for a
base class.

~~~
vtk_object_factory_declare(
  BASE      <base>
  OVERRIDE  <implementation>)
~~~
#]==]
function (vtk_object_factory_declare)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_object_factory_declare
    ""
    "BASE;OVERRIDE"
    "")

  if (_vtk_object_factory_declare_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_object_factory_declare: "
      "${_vtk_object_factory_declare_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_object_factory_declare_BASE)
    message(FATAL_ERROR
      "The `BASE` argument is required.")
  endif ()

  if (NOT DEFINED _vtk_object_factory_declare_OVERRIDE)
    message(FATAL_ERROR
      "The `OVERRIDE` argument is required.")
  endif ()

  set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND
    PROPERTY
      _vtk_object_factory_overrides "${_vtk_object_factory_declare_OVERRIDE}")
  set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND
    PROPERTY
      "_vtk_object_factory_override_${_vtk_object_factory_declare_OVERRIDE}" "${_vtk_object_factory_declare_BASE}")
endfunction ()

#[==[
@brief Generate source for overrides in a module

A module may only have a single declaration of all its object factory
implementations. This function generates the source for all of the overrides
declared using @ref vtk_object_factory_declare.

~~~
vtk_object_factory_configure(
  SOURCE_FILE <variable>
  [HEADER_FILE <variable>]
  [EXPORT_MACRO <macro>]
  [INITIAL_CODE <code>]
  [INITIAL_CODE_FILE <path>]
  [EXTRA_INCLUDES <include>...])
~~~

  - `SOURCE_FILE`: (Required) A variable to set to the path to generated source
    file.
  - `HEADER_FILE`: (Recommended) A variable to set to the path to generated
    header file. This should not be treated as a public header.
  - `EXPORT_MACRO`: (Recommended) The export macro to add to the generated
    class.
  - `INITIAL_CODE`: C++ code to run when the object factory is initialized.
  - `INITIAL_CODE_FILE`: Read `INITIAL_CODE` from a file.
  - `EXTRA_INCLUDES`: A list of headers to include. The header names need to
    include the `<>` or `""` quoting.
#]==]
function (vtk_object_factory_configure)
  if (NOT DEFINED _vtk_build_module)
    message(FATAL_ERROR
      "The `vtk_object_factory_configure` function needs to be run within a module context.")
  endif ()

  cmake_parse_arguments(PARSE_ARGV 0 _vtk_object_factory_configure
    ""
    "SOURCE_FILE;HEADER_FILE;INITIAL_CODE;EXPORT_MACRO;INITIAL_CODE_FILE"
    "EXTRA_INCLUDES")

  if (_vtk_object_factory_configure_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_object_factory_configure: "
      "${_vtk_object_factory_configure_UNPARSED_ARGUMENTS}")
  endif ()

  if (DEFINED _vtk_object_factory_configure_INITIAL_CODE AND
      DEFINED _vtk_object_factory_configure_INITIAL_CODE_FILE)
    message(FATAL_ERROR
      "At most one of `INITIAL_CODE` and `INITIAL_CODE_FILE` may be specified "
      "at a time.")
  endif ()

  if (_vtk_object_factory_configure_INITIAL_CODE_FILE)
    file(READ "${_vtk_object_factory_configure_INITIAL_CODE_FILE}"
      _vtk_object_factory_configure_INITIAL_CODE)
  endif ()

  get_property(_vtk_object_factory_done
    GLOBAL
    PROPERTY "_vtk_object_factory_${_vtk_build_module}"
    SET)
  if (_vtk_object_factory_done)
    message(FATAL_ERROR
      "An object factory has already been created for ${_vtk_build_module}.")
  else ()
    set_property(GLOBAL
      PROPERTY
        "_vtk_object_factory_${_vtk_build_module}" 1)
  endif ()

  get_property(_vtk_object_factory_overrides
    DIRECTORY
    PROPERTY _vtk_object_factory_overrides)

  if (NOT _vtk_object_factory_overrides)
    message(WARNING
      "The ${_vtk_build_module} is generating an object factory, but does not have any declared overrides.")
  endif ()

  set(_vtk_object_factory_doc
    "Override for ${_vtk_build_module} module")

  set(_vtk_object_factory_includes "")
  set(_vtk_object_factory_functions "")
  set(_vtk_object_factory_calls "")

  foreach (_vtk_object_factory_extra_include IN LISTS _vtk_object_factory_configure_EXTRA_INCLUDES)
    string(APPEND _vtk_object_factory_includes
      "#include ${_vtk_object_factory_extra_include}\n")
  endforeach ()

  foreach (_vtk_object_factory_override IN LISTS _vtk_object_factory_overrides)
    get_property(_vtk_object_factory_base
      DIRECTORY
      PROPERTY "_vtk_object_factory_override_${_vtk_object_factory_override}")
    string(APPEND _vtk_object_factory_includes
      "#include \"${_vtk_object_factory_override}.h\"\n")
    string(APPEND _vtk_object_factory_functions
      "VTK_CREATE_CREATE_FUNCTION(${_vtk_object_factory_override})\n")
    string(APPEND _vtk_object_factory_calls
      "this->RegisterOverride(\"${_vtk_object_factory_base}\", \"${_vtk_object_factory_override}\", \"${_vtk_object_factory_doc}\", 1, vtkObjectFactoryCreate${_vtk_object_factory_override});\n")
  endforeach ()

  get_property(_vtk_object_factory_library_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_library_name")

  set(_vtk_object_factory_overrides_header
    "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_object_factory_library_name}ObjectFactory.h")
  set(_vtk_object_factory_overrides_source
    "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_object_factory_library_name}ObjectFactory.cxx")

  configure_file(
    "${_vtkObjectFactory_source_dir}/vtkObjectFactory.h.in"
    "${_vtk_object_factory_overrides_header}"
    @ONLY)
  configure_file(
    "${_vtkObjectFactory_source_dir}/vtkObjectFactory.cxx.in"
    "${_vtk_object_factory_overrides_source}"
    @ONLY)

  if (_vtk_object_factory_configure_HEADER_FILE)
    set("${_vtk_object_factory_configure_HEADER_FILE}"
      "${_vtk_object_factory_overrides_header}"
      PARENT_SCOPE)
  endif ()

  set("${_vtk_object_factory_configure_SOURCE_FILE}"
    "${_vtk_object_factory_overrides_source}"
    PARENT_SCOPE)
endfunction ()
