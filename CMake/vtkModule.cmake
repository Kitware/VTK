get_filename_component(_vtkModule_dir "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)

#[==[.rst:
*********
vtkModule
*********

#]==]
#[==[.rst:

.. cmake:command:: _vtk_module_debug

  Conditionally output debug statements
  |module-internal|

  The :cmake:command:`_vtk_module_debug` function is provided to assist in debugging. It is
  controlled by the `_vtk_module_log` variable which contains a list of "domains"
  to debug.

  .. code-block:: cmake

    _vtk_module_debug(<domain> <format>)


  If the `domain` is enabled for debugging, the `format` argument is configured
  and printed. It should contain `@` variable expansions to replace rather than
  it being done outside. This helps to avoid the cost of generating large strings
  when debugging is disabled.
#]==]
function (_vtk_module_debug domain format)
  if (NOT _vtk_module_log STREQUAL "ALL" AND
      NOT domain IN_LIST _vtk_module_log)
    return ()
  endif ()

  string(CONFIGURE "${format}" _vtk_module_debug_msg)
  if (_vtk_module_debug_msg)
    message(STATUS
      "VTK module debug ${domain}: ${_vtk_module_debug_msg}")
  endif ()
endfunction ()

# TODO: Support finding `vtk.module` and `vtk.kit` contents in the
# `CMakeLists.txt` files for the module via a comment header.

#[==[.rst:

.. cmake:command:: vtk_module_find_kits

  Find `vtk.kit` files in a set of directories
  |module|

  .. code-block:: cmake

     vtk_module_find_kits(<output> [<directory>...])


  This scans the given directories recursively for `vtk.kit` files and put the
  paths into the output variable.
#]==]
function (vtk_module_find_kits output)
  set(_vtk_find_kits_all)
  foreach (_vtk_find_kits_directory IN LISTS ARGN)
    file(GLOB_RECURSE _vtk_find_kits_kits
      "${_vtk_find_kits_directory}/vtk.kit")
    list(APPEND _vtk_find_kits_all
      ${_vtk_find_kits_kits})
  endforeach ()
  set("${output}" ${_vtk_find_kits_all} PARENT_SCOPE)
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_find_modules

  Find `vtk.module` files in a set of directories
  |module|

  .. code-block:: cmake

     vtk_module_find_modules(<output> [<directory>...])

  This scans the given directories recursively for ``vtk.module`` files and put the
  paths into the output variable. Note that module files are assumed to live next
  to the ``CMakeLists.txt`` file which will build the module.
#]==]
function (vtk_module_find_modules output)
  set(_vtk_find_modules_all)
  foreach (_vtk_find_modules_directory IN LISTS ARGN)
    file(GLOB_RECURSE _vtk_find_modules_modules
      "${_vtk_find_modules_directory}/vtk.module")
    list(APPEND _vtk_find_modules_all
      ${_vtk_find_modules_modules})
  endforeach ()
  set("${output}" ${_vtk_find_modules_all} PARENT_SCOPE)
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_split_module_name

 Split a module name into a namespace and target component
 |module-internal|

 Module names may include a namespace. This function splits the name into a
 namespace and target name part.

 .. code-block:: cmake

    _vtk_module_split_module_name(<name> <prefix>)

 The ``<prefix>_NAMESPACE`` and ``<prefix>_TARGET_NAME`` variables will be set in
 the calling scope.
#]==]
function (_vtk_module_split_module_name name prefix)
  string(FIND "${name}" "::" namespace_pos)
  if (namespace_pos EQUAL -1)
    set(namespace "")
    set(target_name "${name}")
  else ()
    string(SUBSTRING "${name}" 0 "${namespace_pos}" namespace)
    math(EXPR name_pos "${namespace_pos} + 2")
    string(SUBSTRING "${name}" "${name_pos}" -1 target_name)
  endif ()

  set("${prefix}_NAMESPACE"
    "${namespace}"
    PARENT_SCOPE)
  set("${prefix}_TARGET_NAME"
    "${target_name}"
    PARENT_SCOPE)
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_optional_dependency_exists

 Detect whether an optional dependency exists or not.
 |module-internal|

 Optional dependencies need to be detected
 namespace and target name part.

 .. code-block:: cmake

    _vtk_module_optional_dependency_exists(<dependency>
      SATISFIED_VAR <var>)

 The result will be returned in the variable specified by ``SATISFIED_VAR``.
#]==]
function (_vtk_module_optional_dependency_exists dependency)
  cmake_parse_arguments(_vtk_optional_dep
    ""
    "SATISFIED_VAR;PACKAGE"
    ""
    ${ARGN})

  if (_vtk_optional_dep_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for `_vtk_module_optional_dependency_exists`: "
      "${_vtk_optional_dep_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_optional_dep_SATISFIED_VAR)
    message(FATAL_ERROR
      "The `SATISFIED_VAR` argument is required.")
  endif ()

  set(_vtk_optional_dep_satisfied 0)
  if (TARGET "${dependency}")
    # If the target is imported, we check its `_FOUND` variable. If it is not
    # imported, we assume it is set up properly as a normal target (or an
    # `ALIAS`).
    get_property(_vtk_optional_dep_is_imported
      TARGET    "${dependency}"
      PROPERTY  IMPORTED)
    if (_vtk_optional_dep_is_imported)
      _vtk_module_split_module_name("${dependency}" _vtk_optional_dep_parse)
      set(_vtk_optional_dep_found_var
        "${_vtk_optional_dep_parse_NAMESPACE}_${_vtk_optional_dep_parse_TARGET_NAME}_FOUND")
      if (DEFINED "${_vtk_optional_dep_found_var}" AND
          ${_vtk_optional_dep_found_var})
        set(_vtk_optional_dep_satisfied 1)
      endif ()
    else ()
      set(_vtk_optional_dep_satisfied 1)
    endif ()
  endif ()

  set("${_vtk_optional_dep_SATISFIED_VAR}"
    "${_vtk_optional_dep_satisfied}"
    PARENT_SCOPE)
endfunction ()

#[==[.rst:

.. _module-parse-module:

vtk.module file contents
========================

The `vtk.module` file is parsed and used as arguments to a CMake function which
stores information about the module for use when building it. Note that no
variable expansion is allowed and it is not CMake code, so no control flow is
allowed. Comments are supported and any content after a `#` on a line is
treated as a comment. Due to the breakdown of the content, quotes are not
meaningful within the files.

Example:

.. code-block:: cmake

  NAME
    VTK::CommonCore
  LIBRARY_NAME
    vtkCommonCore
  DESCRIPTION
    The base VTK library.
  LICENSE_FILES
    Copyright.txt
  SPDX_COPYRIGHT_TEXT
    Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  SPDX_LICENSE_IDENTIFIER
    BSD-3-Clause
  GROUPS
    StandAlone
  DEPENDS
    VTK::kwiml
  PRIVATE_DEPENDS
    VTK::vtksys
    VTK::utf8

All values are optional unless otherwise noted. The following arguments are
supported:

* ``NAME``: (Required) The name of the module.
* ``LIBRARY_NAME``: The base name of the library file. It defaults to the
  module name, but any namespaces are removed. For example, a ``NS::Foo``
  module will have a default ``LIBRARY_NAME`` of ``Foo``.
* ``DESCRIPTION``: (Recommended) Short text describing what the module is for.
* ``KIT``: The name of the kit the module belongs to (see ``Kits files`` for more
  information).
* ``IMPLEMENTABLE``: If present, the module contains logic which supports the
  autoinit functionality.
* ``GROUPS``: Modules may belong to "groups" which is exposed as a build
  option. This allows for enabling a set of modules with a single build
  option.
* ``CONDITION``: Arguments to CMake's ``if`` command which may be used to hide
  the module for certain platforms or other reasons. If the expression is
  false, the module is completely ignored.
* ``DEPENDS``: A list of modules which are required by this module and modules
  using this module.
* ``PRIVATE_DEPENDS``: A list of modules which are required by this module, but
  not by those using this module.
* ``OPTIONAL_DEPENDS``: A list of modules which are used by this module if
  enabled; these are treated as ``PRIVATE_DEPENDS`` if they exist.
* ``ORDER_DEPENDS``: Dependencies which only matter for ordering. This does not
  mean that the module will be enabled, just guaranteed to build before this
  module.
* ``IMPLEMENTS``: A list of modules for which this module needs to register
  with.
* ``TEST_DEPENDS``: Modules required by the test suite for this module.
* ``TEST_OPTIONAL_DEPENDS``: Modules used by the test suite for this module if
  available.
* ``TEST_LABELS``: Labels to apply to the tests of this module. By default, the
  module name is applied as a label.
* ``EXCLUDE_WRAP``: If present, this module should not be wrapped in any
  language.
* ``INCLUDE_MARSHAL``: If present, this module opts into automatic code generation
  of (de)serializers. This option requires that the module is not excluded from wrapping
  with `EXCLUDE_WRAP`.
* ``THIRD_PARTY``: If present, this module is a third party module.
* ``LICENSE_FILES``: A list of license files to install for the module.
* ``SPDX_LICENSE_IDENTIFIER``: A license identifier for SPDX file generation.
* ``SPDX_DOWNLOAD_LOCATION``: A download location for the SPDX file generation.
* ``SPDX_COPYRIGHT_TEXT``: A copyright text for the SPDX file generation.
* ``SPDX_CUSTOM_LICENSE_FILE``: A relative path to a  single custom license file to include in generated SPDX file.
* ``SPDX_CUSTOM_LICENSE_NAME``: The name of the single custom license, without the ``LicenseRef-``

#]==]

#[==[.rst:
.. cmake:command:: _vtk_module_parse_module_args

  Parse ``vtk.module`` file contents

  |module-impl|

  This macro places all ``vtk.module`` keyword "arguments" into the caller's scope
  prefixed with the value of ``name_output`` which is set to the ``NAME`` of the
  module.

  .. code-block:: cmake

      _vtk_module_parse_module_args(name_output <vtk.module args...>)

  For example, this ``vtk.module`` file:

  .. code-block:: cmake

    NAME
      Namespace::Target
    LIBRARY_NAME
      nsTarget

  called with ``_vtk_module_parse_module_args(name ...)`` will set the following
  variables in the calling scope:

  - ``name``: ``Namespace::Target``
  - ``Namespace::Target_LIBRARY_NAME``: ``nsTarget``

  With namespace support for module names, the variable should instead be
  referenced via ``${${name}_LIBRARY_NAME}`` instead.
#]==]
macro (_vtk_module_parse_module_args name_output)
  cmake_parse_arguments("_name"
    ""
    "NAME"
    ""
    ${ARGN})

  if (NOT _name_NAME)
    message(FATAL_ERROR
      "A VTK module requires a name (from ${_vtk_scan_module_file}).")
  endif ()
  set("${name_output}" "${_name_NAME}")

  cmake_parse_arguments("${_name_NAME}"
    "IMPLEMENTABLE;EXCLUDE_WRAP;THIRD_PARTY;INCLUDE_MARSHAL"
    "LIBRARY_NAME;NAME;KIT;SPDX_DOWNLOAD_LOCATION;SPDX_CUSTOM_LICENSE_FILE;SPDX_CUSTOM_LICENSE_NAME"
    "GROUPS;DEPENDS;PRIVATE_DEPENDS;OPTIONAL_DEPENDS;ORDER_DEPENDS;TEST_DEPENDS;TEST_OPTIONAL_DEPENDS;TEST_LABELS;DESCRIPTION;CONDITION;IMPLEMENTS;LICENSE_FILES;SPDX_LICENSE_IDENTIFIER;SPDX_COPYRIGHT_TEXT"
    ${ARGN})

  if (${_name_NAME}_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for ${_name_NAME}: "
      "${${_name_NAME}_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT ${_name_NAME}_DESCRIPTION AND _vtk_module_warnings)
    message(WARNING "The ${_name_NAME} module should have a description")
  endif ()
  string(REPLACE ";" " " "${_name_NAME}_DESCRIPTION" "${${_name_NAME}_DESCRIPTION}")

  _vtk_module_split_module_name("${_name_NAME}" "${_name_NAME}")

  if (NOT DEFINED "${_name_NAME}_LIBRARY_NAME")
    set("${_name_NAME}_LIBRARY_NAME" "${${_name_NAME}_TARGET_NAME}")
  endif ()

  if (NOT ${_name_NAME}_LIBRARY_NAME)
    message(FATAL_ERROR "The ${_name_NAME} module must have a non-empty `LIBRARY_NAME`.")
  endif ()

  list(APPEND "${_name_NAME}_TEST_LABELS"
    "${${_name_NAME}_NAME}"
    "${${_name_NAME}_LIBRARY_NAME}")
endmacro ()

#[==[.rst:
.. _module-parse-kit:

vtk.kit file contents
=====================

The `vtk.kit` file is parsed similarly to `vtk.module` files. Kits are intended
to bring together related modules into a single library in order to reduce the
number of objects that linkers need to deal with.

Example:

.. code-block:: cmake

  NAME
    VTK::Common
  LIBRARY_NAME
    vtkCommon
  DESCRIPTION
    Core utilities for VTK.

All values are optional unless otherwise noted. The following arguments are
supported:

* ``NAME``: (Required) The name of the kit.
* ``LIBRARY_NAME``: The base name of the library file. It defaults to the
  module name, but any namespaces are removed. For example, a ``NS::Foo``
  module will have a default ``LIBRARY_NAME`` of ``Foo``.
* ``DESCRIPTION``: (Recommended) Short text describing what the kit contains.
#]==]

#[==[.rst:
.. cmake:command:: _vtk_module_parse_kit_args

  Parse `vtk.kit` file contents
  |module-impl|

  Just like :cmake:command:`_vtk_module_parse_module_args`, but for kits.
#]==]
macro (_vtk_module_parse_kit_args name_output)
  cmake_parse_arguments("_name"
    ""
    "NAME"
    ""
    ${ARGN})

  if (NOT _name_NAME)
    message(FATAL_ERROR
      "A VTK kit requires a name (from ${_vtk_scan_kit_file}).")
  endif ()
  set("${name_output}" "${_name_NAME}")

  cmake_parse_arguments("${_name_NAME}"
    ""
    "NAME;LIBRARY_NAME"
    "DESCRIPTION"
    ${ARGN})

  if (${_name_NAME}_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for ${_name_NAME}: "
      "${${_name_NAME}_UNPARSED_ARGUMENTS}")
  endif ()

  _vtk_module_split_module_name("${_name_NAME}" "${_name_NAME}")

  if (NOT DEFINED "${_name_NAME}_LIBRARY_NAME")
    set("${_name_NAME}_LIBRARY_NAME" "${${_name_NAME}_TARGET_NAME}")
  endif ()

  if (NOT ${_name_NAME}_LIBRARY_NAME)
    message(FATAL_ERROR "The ${_name_NAME} module must have a non-empty `LIBRARY_NAME`.")
  endif ()

  if (NOT ${_name_NAME}_DESCRIPTION AND _vtk_module_warnings)
    message(WARNING "The ${_name_NAME} kit should have a description")
  endif ()
  string(REPLACE ";" " " "${_name_NAME}_DESCRIPTION" "${${_name_NAME}_DESCRIPTION}")
endmacro ()

#[==[.rst:
.. _module-enable-status:

Enable status values
====================

Modules and groups are enable and disable preferences are specified using a
5-way flag setting:

- ``YES``: The module or group must be built.
- ``NO``: The module or group must not be built.
- ``WANT``: The module or group should be built if possible.
- ``DONT_WANT``: The module or group should only be built if required (e.g.,
  via a dependency).
- ``DEFAULT``: Acts as either ``WANT`` or ``DONT_WANT`` based on the group settings
  for the module or ``WANT_BY_DEFAULT`` option to :cmake:command:`vtk_module_scan` if no
  other preference is specified. This is usually handled via another setting
  in the main project.

If a ``YES`` module preference requires a module with a ``NO`` preference, an error
is raised.

A module with a setting of ``DEFAULT`` will look for its first non-``DEFAULT``
group setting and only if all of those are set to ``DEFAULT`` is the
``WANT_BY_DEFAULT`` setting used.
#]==]

#[==[.rst:
.. cmake:command:: _vtk_module_verify_enable_value

  Verify enable values
  |module-impl|

  Verifies that the variable named as the first parameter is a valid `enable
  status` value.

  .. code-block:: cmake

    _vtk_module_verify_enable_value(var)
#]==]
function (_vtk_module_verify_enable_value var)
  if (NOT (${var} STREQUAL "YES" OR
           ${var} STREQUAL "WANT" OR
           ${var} STREQUAL "DONT_WANT" OR
           ${var} STREQUAL "NO" OR
           ${var} STREQUAL "DEFAULT"))
    message(FATAL_ERROR
      "The `${var}` variable must be one of `YES`, `WANT`, `DONT_WANT`, `NO`, "
      "or `DEFAULT`. Found `${${var}}`.")
  endif ()
endfunction ()

include("${CMAKE_CURRENT_LIST_DIR}/vtkTopologicalSort.cmake")

#[==[.rst:
.. cmake:command:: vtk_module_scan

  Scan modules and kits
  |module|

  Once all of the modules and kits files have been found, they are "scanned" to
  determine what modules are enabled or required.

  .. code-block:: cmake

    vtk_module_scan(
      MODULE_FILES              <file>...
      [KIT_FILES                <file>...]
      PROVIDES_MODULES          <variable>
      [PROVIDES_KITS            <variable>]
      [REQUIRES_MODULES         <variable>]
      [REQUEST_MODULES          <module>...]
      [REJECT_MODULES           <module>...]
      [UNRECOGNIZED_MODULES     <variable>]
      [WANT_BY_DEFAULT          <ON|OFF>]
      [HIDE_MODULES_FROM_CACHE  <ON|OFF>]
      [ENABLE_TESTS             <ON|OFF|WANT|DEFAULT>])

  The ``MODULE_FILES`` and ``PROVIDES_MODULES`` arguments are required. Modules which
  refer to kits must be scanned at the same time as their kits. This is so that
  modules may not add themselves to kits declared prior. The arguments are as follows:

  * ``MODULE_FILES``: (Required) The list of module files to scan.
  * ``KIT_FILES``: The list of kit files to scan.
  * ``PROVIDES_MODULES``: (Required) This variable will contain the list of
    modules which are enabled due to this scan.
  * ``PROVIDES_KITS``: (Required if ``KIT_FILES`` are provided) This variable will
    contain the list of kits which are enabled due to this scan.
  * ``REQUIRES_MODULES``: This variable will contain the list of modules required
    by the enabled modules that were not scanned.
  * ``REQUEST_MODULES``: The list of modules required by previous scans.
  * ``REJECT_MODULES``: The list of modules to exclude from the scan. If any of
    these modules are required, an error will be raised.
  * ``UNRECOGNIZED_MODULES``: This variable will contain the list of requested
    modules that were not scanned.
  * ``WANT_BY_DEFAULT``: (Defaults to ``OFF``) Whether modules should default to
    being built or not.
  * ``HIDE_MODULES_FROM_CACHE``: (Defaults to ``OFF``) Whether or not to hide the
    control variables from the cache or not. If enabled, modules will not be
    built unless they are required elsewhere.
  * ``ENABLE_TESTS``: (Defaults to ``DEFAULT``) Whether or not modules required by
    the tests for the scanned modules should be enabled or not.

    - ``ON``: Modules listed as ``TEST_DEPENDS`` will be required.
    - ``OFF``: Test modules will not be considered.
    - ``WANT``: Test dependencies will enable modules if possible. Note that this
      has known issues where modules required only via testing may not have
      their dependencies enabled.
    - ``DEFAULT``: Test modules will be enabled if their required dependencies
      are satisfied and skipped otherwise.

  To make error messages clearer, modules passed to ``REQUIRES_MODULES`` and
  ``REJECT_MODULES`` may have a ``_vtk_module_reason_<MODULE>`` variable set to the
  reason for the module appearing in either argument. For example, if the
  ``Package::Frobnitz`` module is required due to a ``ENABLE_FROBNITZ`` cache
  variable:

  .. code-block:: cmake

    set("_vtk_module_reason_Package::Frobnitz"
      "via the `ENABLE_FROBNITZ` setting")

  Additionally, the reason for the ``WANT_BY_DEFAULT`` value may be provided via
  the ``_vtk_module_reason_WANT_BY_DEFAULT`` variable.

.. _module-scanning-multiple:

Scanning multiple groups of modules
===================================

When scanning complicated projects, multiple scans may be required to get
defaults set properly. The ``REQUIRES_MODULES``, ``REQUEST_MODULES``, and
``UNRECOGNIZED_MODULES`` arguments are meant to deal with this case. As an
example, imagine a project with its source code, third party dependencies, as
well as some utility modules which should only be built as necessary. Here, the
project would perform three scans, one for each "grouping" of modules:

.. code-block:: cmake

  # Scan our modules first because we need to know what of the other groups we
  # need.
  vtk_module_find_modules(our_modules "${CMAKE_CURRENT_SOURCE_DIR}/src")
  vtk_module_scan(
    MODULE_FILES      ${our_modules}
    PROVIDES_MODULES  our_enabled_modules
    REQUIRES_MODULES  required_modules)

  # Scan the third party modules, requesting only those that are necessary, but
  # allowing them to be toggled during the build.
  vtk_module_find_modules(third_party_modules "${CMAKE_CURRENT_SOURCE_DIR}/third-party")
  vtk_module_scan(
    MODULE_FILES            ${third_party_modules}
    PROVIDES_MODULES        third_party_enabled_modules
    # These modules were requested by an earlier scan.
    REQUEST_MODULES         ${required_modules}
    REQUIRES_MODULES        required_modules
    UNRECOGNIZED_MODULES    unrecognized_modules)

  # These modules are internal and should only be built if necessary. There is no
  # need to support them being enabled independently, so hide them from the
  # cache.
  vtk_module_find_modules(utility_modules "${CMAKE_CURRENT_SOURCE_DIR}/utilities")
  vtk_module_scan(
    MODULE_FILES            ${utility_modules}
    PROVIDES_MODULES        utility_enabled_modules
    # These modules were either requested or unrecognized by an earlier scan.
    REQUEST_MODULES         ${required_modules}
                            ${unrecognized_modules}
    REQUIRES_MODULES        required_modules
    UNRECOGNIZED_MODULES    unrecognized_modules
    HIDE_MODULES_FROM_CACHE ON)

  if (required_modules OR unrecognized_modules)
    # Not all of the modules we required were found. This should probably error out.
  endif ()
#]==]
function (vtk_module_scan)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_scan
    ""
    "WANT_BY_DEFAULT;HIDE_MODULES_FROM_CACHE;PROVIDES_MODULES;REQUIRES_MODULES;UNRECOGNIZED_MODULES;ENABLE_TESTS;PROVIDES_KITS"
    "MODULE_FILES;KIT_FILES;REQUEST_MODULES;REJECT_MODULES")

  if (_vtk_scan_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_scan: "
      "${_vtk_scan_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_scan_WANT_BY_DEFAULT)
    set(_vtk_scan_WANT_BY_DEFAULT OFF)
  endif ()

  if (NOT DEFINED _vtk_scan_HIDE_MODULES_FROM_CACHE)
    set(_vtk_scan_HIDE_MODULES_FROM_CACHE OFF)
  endif ()

  if (NOT DEFINED _vtk_scan_PROVIDES_MODULES)
    message(FATAL_ERROR
      "The `PROVIDES_MODULES` argument is required.")
  endif ()

  if (NOT DEFINED _vtk_scan_PROVIDES_KITS AND _vtk_scan_KIT_FILES)
    message(FATAL_ERROR
      "The `PROVIDES_KITS` argument is required.")
  endif ()

  if (NOT DEFINED _vtk_scan_ENABLE_TESTS)
    set(_vtk_scan_ENABLE_TESTS "DEFAULT")
  endif ()

  if (NOT (_vtk_scan_ENABLE_TESTS STREQUAL "ON" OR
           _vtk_scan_ENABLE_TESTS STREQUAL "OFF" OR
           _vtk_scan_ENABLE_TESTS STREQUAL "WANT" OR
           _vtk_scan_ENABLE_TESTS STREQUAL "DEFAULT"))
    message(FATAL_ERROR
      "The `ENABLE_TESTS` argument must be one of `ON`, `OFF`, `WANT`, or "
      "`DEFAULT`. " "Received `${_vtk_scan_ENABLE_TESTS}`.")
  endif ()

  if (NOT _vtk_scan_MODULE_FILES)
    message(FATAL_ERROR
      "No module files given to scan.")
  endif ()

  set(_vtk_scan_option_default_type STRING)
  if (_vtk_scan_HIDE_MODULES_FROM_CACHE)
    set(_vtk_scan_option_default_type INTERNAL)
  endif ()

  set(_vtk_scan_all_kits)

  foreach (_vtk_scan_kit_file IN LISTS _vtk_scan_KIT_FILES)
    if (NOT IS_ABSOLUTE "${_vtk_scan_kit_file}")
      string(PREPEND _vtk_scan_kit_file "${CMAKE_CURRENT_SOURCE_DIR}/")
    endif ()
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND
      PROPERTY
        CMAKE_CONFIGURE_DEPENDS "${_vtk_scan_kit_file}")

    file(READ "${_vtk_scan_kit_file}" _vtk_scan_kit_args)
    # Replace comments.
    string(REGEX REPLACE "#[^\n]*\n" "\n" _vtk_scan_kit_args "${_vtk_scan_kit_args}")
    # Use argument splitting.
    string(REGEX REPLACE "( |\n)+" ";" _vtk_scan_kit_args "${_vtk_scan_kit_args}")
    _vtk_module_parse_kit_args(_vtk_scan_kit_name ${_vtk_scan_kit_args})
    _vtk_module_debug(kit "@_vtk_scan_kit_name@ declared by @_vtk_scan_kit_file@")

    list(APPEND _vtk_scan_all_kits
      "${_vtk_scan_kit_name}")

    # Set properties for building.
    set_property(GLOBAL
      PROPERTY
        "_vtk_kit_${_vtk_scan_kit_name}_namespace" "${${_vtk_scan_kit_name}_NAMESPACE}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_kit_${_vtk_scan_kit_name}_target_name" "${${_vtk_scan_kit_name}_TARGET_NAME}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_kit_${_vtk_scan_kit_name}_library_name" "${${_vtk_scan_kit_name}_LIBRARY_NAME}")
  endforeach ()

  set(_vtk_scan_all_modules)
  set(_vtk_scan_all_groups)
  set(_vtk_scan_rejected_modules)

  # Read all of the module files passed in.
  foreach (_vtk_scan_module_file IN LISTS _vtk_scan_MODULE_FILES)
    if (NOT IS_ABSOLUTE "${_vtk_scan_module_file}")
      string(PREPEND _vtk_scan_module_file "${CMAKE_CURRENT_SOURCE_DIR}/")
    endif ()
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND
      PROPERTY
        CMAKE_CONFIGURE_DEPENDS "${_vtk_scan_module_file}")

    file(READ "${_vtk_scan_module_file}" _vtk_scan_module_args)
    # Replace comments.
    string(REGEX REPLACE "#[^\n]*\n" "\n" _vtk_scan_module_args "${_vtk_scan_module_args}")
    # Use argument splitting.
    string(REGEX REPLACE "( |\n)+" ";" _vtk_scan_module_args "${_vtk_scan_module_args}")
    _vtk_module_parse_module_args(_vtk_scan_module_name ${_vtk_scan_module_args})
    _vtk_module_debug(module "@_vtk_scan_module_name@ declared by @_vtk_scan_module_file@")
    string(REPLACE "::" "_" _vtk_scan_module_name_safe "${_vtk_scan_module_name}")

    if (${_vtk_scan_module_name}_THIRD_PARTY)
      if (_vtk_module_warnings)
        if (${_vtk_scan_module_name}_EXCLUDE_WRAP)
          message(WARNING
            "The third party ${_vtk_scan_module_name} module does not need to "
            "declare `EXCLUDE_WRAP` also.")
        endif ()
      endif ()
      if (${_vtk_scan_module_name}_INCLUDE_MARSHAL)
        message(FATAL_ERROR
          "The third party ${_vtk_scan_module_name} module may not opt-in to "
          "`INCLUDE_MARSHAL`.")
      endif ()
      if (${_vtk_scan_module_name}_IMPLEMENTABLE)
        message(FATAL_ERROR
          "The third party ${_vtk_scan_module_name} module may not be "
          "`IMPLEMENTABLE`.")
      endif ()
      if (${_vtk_scan_module_name}_IMPLEMENTS)
        message(FATAL_ERROR
          "The third party ${_vtk_scan_module_name} module may not "
          "`IMPLEMENTS` another module.")
      endif ()
      if (${_vtk_scan_module_name}_KIT)
        message(FATAL_ERROR
          "The third party ${_vtk_scan_module_name} module may not be part of "
          "a kit (${${_vtk_scan_module_name}_KIT}).")
      endif ()
    endif ()

    # Throw error when mutually exclusive options are present.
    # When a module opts into marshalling, it requires the hierarchy
    if (${_vtk_scan_module_name}_EXCLUDE_WRAP AND ${_vtk_scan_module_name}_INCLUDE_MARSHAL)
      message(FATAL_ERROR
        "The ${_vtk_scan_module_name} module can not declare `EXCLUDE_WRAP` and `INCLUDE_MARSHAL` at the same time.")
    endif ()
    if (${_vtk_scan_module_name}_KIT)
      if (NOT ${_vtk_scan_module_name}_KIT IN_LIST _vtk_scan_all_kits)
        message(FATAL_ERROR
          "The ${_vtk_scan_module_name} belongs to the "
          "${${_vtk_scan_module_name}_KIT} kit, but it has not been scanned.")
      endif ()
    endif ()

    # Check if the module is visible. Modules which have a failing condition
    # are basically invisible.
    if (DEFINED ${_vtk_scan_module_name}_CONDITION)
      if (NOT (${${_vtk_scan_module_name}_CONDITION}))
        if (DEFINED "VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe}")
          set_property(CACHE "VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe}"
            PROPERTY
              TYPE INTERNAL)
        endif ()
        _vtk_module_debug(module "@_vtk_scan_module_name@ hidden by its `CONDITION`")
        continue ()
      endif ()
    endif ()

    # Determine whether we should provide a user-visible option for this
    # module.
    set(_vtk_build_use_option 1)
    if (DEFINED _vtk_scan_REQUEST_MODULE)
      if (_vtk_scan_module_name IN_LIST _vtk_scan_REQUEST_MODULE)
        set("_vtk_scan_enable_${_vtk_scan_module_name}" YES)
        set(_vtk_build_use_option 0)
      endif ()
    endif ()
    if (DEFINED _vtk_scan_REJECT_MODULES)
      if (_vtk_scan_module_name IN_LIST _vtk_scan_REJECT_MODULES)
        if (NOT _vtk_build_use_option)
          message(FATAL_ERROR
            "The ${_vtk_scan_module_name} module has been requested and rejected.")
        endif ()
        # Rejected modules should not have a build option.
        set(_vtk_build_use_option 0)
        list(APPEND _vtk_scan_rejected_modules
          "${_vtk_scan_module_name}")
      endif ()
    endif ()

    # Handle cache entries and determine the enabled state of the module from
    # the relevant cache variables.
    if (_vtk_build_use_option)
      set("VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe}" "DEFAULT"
        CACHE STRING "Enable the ${_vtk_scan_module_name} module. ${${_vtk_scan_module_name}_DESCRIPTION}")
      mark_as_advanced("VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe}")
      set_property(CACHE "VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe}"
        PROPERTY
          STRINGS "YES;WANT;DONT_WANT;NO;DEFAULT")
      _vtk_module_verify_enable_value("VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe}")

      if (NOT VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe} STREQUAL "DEFAULT")
        set("_vtk_scan_enable_${_vtk_scan_module_name}" "${VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe}}")
        set("_vtk_scan_enable_reason_${_vtk_scan_module_name}"
          "via `VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe}`")
        _vtk_module_debug(enable "@_vtk_scan_module_name@ is `${_vtk_scan_enable_${_vtk_scan_module_name}}` by cache value")
      endif ()

      # Check the state of any groups the module belongs to.
      foreach (_vtk_scan_group IN LISTS "${_vtk_scan_module_name}_GROUPS")
        if (NOT DEFINED "VTK_GROUP_ENABLE_${_vtk_scan_group}")
          set(_vtk_scan_group_default "DEFAULT")
          if (DEFINED "_vtk_module_group_default_${_vtk_scan_group}")
            set(_vtk_scan_group_default "${_vtk_module_group_default_${_vtk_scan_group}}")
          endif ()
          set("VTK_GROUP_ENABLE_${_vtk_scan_group}" "${_vtk_scan_group_default}"
            CACHE STRING "Enable the ${_vtk_scan_group} group modules.")
          set_property(CACHE "VTK_GROUP_ENABLE_${_vtk_scan_group}"
            PROPERTY
              STRINGS "YES;WANT;DONT_WANT;NO;DEFAULT")
          set_property(CACHE "VTK_GROUP_ENABLE_${_vtk_scan_group}"
            PROPERTY
              TYPE "${_vtk_scan_option_default_type}")
        endif ()
        _vtk_module_verify_enable_value("VTK_GROUP_ENABLE_${_vtk_scan_group}")

        if (NOT VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe} STREQUAL "DEFAULT")
          continue ()
        endif ()

        # Determine the state of the group.
        set(_vtk_scan_group_enable "${VTK_GROUP_ENABLE_${_vtk_scan_group}}")
        if (NOT _vtk_scan_group_enable STREQUAL "DEFAULT")
          set("_vtk_scan_enable_${_vtk_scan_module_name}" "${_vtk_scan_group_enable}")
          set("_vtk_scan_enable_reason_${_vtk_scan_module_name}"
            "via `VTK_GROUP_ENABLE_${_vtk_scan_group}`")
          _vtk_module_debug(enable "@_vtk_scan_module_name@ is DEFAULT, using group `@_vtk_scan_group@` setting: @_vtk_scan_group_enable@")
        endif ()
      endforeach ()

      set_property(CACHE "VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe}"
        PROPERTY
          TYPE "${_vtk_scan_option_default_type}")
    endif ()

    if (NOT DEFINED "_vtk_scan_enable_${_vtk_scan_module_name}" AND
        VTK_MODULE_ENABLE_${_vtk_scan_module_name_safe} STREQUAL "DEFAULT")
      if (_vtk_scan_WANT_BY_DEFAULT)
        set("_vtk_scan_enable_${_vtk_scan_module_name}" "WANT")
      else ()
        set("_vtk_scan_enable_${_vtk_scan_module_name}" "DONT_WANT")
      endif ()
      if (DEFINED _vtk_module_reason_WANT_BY_DEFAULT)
        set("_vtk_scan_enable_reason_${_vtk_scan_module_name}"
          "${_vtk_module_reason_WANT_BY_DEFAULT}")
      else ()
        set("_vtk_scan_enable_reason_${_vtk_scan_module_name}"
          "via `WANT_BY_DEFAULT=${_vtk_scan_WANT_BY_DEFAULT}`")
      endif ()
      _vtk_module_debug(enable "@_vtk_scan_module_name@ is DEFAULT, using `WANT_BY_DEFAULT`: ${_vtk_scan_enable_reason_${_vtk_scan_module_name}}")
    endif ()

    list(APPEND _vtk_scan_all_modules
      "${_vtk_scan_module_name}")
    set("_vtk_scan_${_vtk_scan_module_name}_all_depends"
      ${${_vtk_scan_module_name}_DEPENDS}
      ${${_vtk_scan_module_name}_PRIVATE_DEPENDS})

    if (${_vtk_scan_module_name}_THIRD_PARTY)
      set("${_vtk_scan_module_name}_INCLUDE_MARSHAL" FALSE)
      set("${_vtk_scan_module_name}_EXCLUDE_WRAP" TRUE)
      set("${_vtk_scan_module_name}_IMPLEMENTABLE" FALSE)
      set("${_vtk_scan_module_name}_IMPLEMENTS")
    endif ()

    if (${_vtk_scan_module_name}_KIT)
      _vtk_module_debug(kit "@_vtk_scan_module_name@ belongs to the ${${_vtk_scan_module_name}_KIT} kit")
    endif ()

    # Set properties for building.
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_file" "${_vtk_scan_module_file}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_namespace" "${${_vtk_scan_module_name}_NAMESPACE}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_target_name" "${${_vtk_scan_module_name}_TARGET_NAME}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_library_name" "${${_vtk_scan_module_name}_LIBRARY_NAME}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_third_party" "${${_vtk_scan_module_name}_THIRD_PARTY}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_exclude_wrap" "${${_vtk_scan_module_name}_EXCLUDE_WRAP}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_kit" "${${_vtk_scan_module_name}_KIT}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_depends" "${${_vtk_scan_module_name}_DEPENDS}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_order_depends" "${${_vtk_scan_module_name}_ORDER_DEPENDS}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_private_depends" "${${_vtk_scan_module_name}_PRIVATE_DEPENDS}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_optional_depends" "${${_vtk_scan_module_name}_OPTIONAL_DEPENDS}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_test_depends" "${${_vtk_scan_module_name}_TEST_DEPENDS}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_test_optional_depends" "${${_vtk_scan_module_name}_TEST_OPTIONAL_DEPENDS}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_test_labels" "${${_vtk_scan_module_name}_TEST_LABELS}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_implements" "${${_vtk_scan_module_name}_IMPLEMENTS}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_implementable" "${${_vtk_scan_module_name}_IMPLEMENTABLE}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_include_marshal" "${${_vtk_scan_module_name}_INCLUDE_MARSHAL}")
    # create absolute path for license files
    set(_license_files)
    foreach (_license_file IN LISTS ${_vtk_scan_module_name}_LICENSE_FILES)
      if (NOT IS_ABSOLUTE "${_license_file}")
        get_filename_component(_vtk_scan_module_dir "${_vtk_scan_module_file}" DIRECTORY)
        string(PREPEND _license_file "${_vtk_scan_module_dir}/")
      endif ()
      list(APPEND _license_files "${_license_file}")
    endforeach ()
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_license_files" "${_license_files}")

    # Revert argument splitting done in vtk_module_scan() just before calling _vtk_module_parse_module_args()
    # For example, it converts
    #   "MIT;AND;(LGPL-2.1-or-later;OR;BSD-3-Clause)"
    # back to
    #   "MIT AND (LGPL-2.1-or-later OR BSD-3-Clause)"
    string(REGEX REPLACE ";" " " _spdx_license_identifier "${${_vtk_scan_module_name}_SPDX_LICENSE_IDENTIFIER}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_spdx_license_identifier" "${_spdx_license_identifier}")

    # Revert argument splitting done in vtk_module_scan() just before calling _vtk_module_parse_module_args()
    # For example, it converts
    #   "Copyright;(c);Ken;Martin,;Will;Schroeder,;Bill;Lorensen"
    # back to
    #   "Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen"
    string(REGEX REPLACE ";" " " _spdx_copyright_text "${${_vtk_scan_module_name}_SPDX_COPYRIGHT_TEXT}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_spdx_copyright_text" "${_spdx_copyright_text}")

    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_spdx_download_location" "${${_vtk_scan_module_name}_SPDX_DOWNLOAD_LOCATION}")

    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_spdx_custom_license_file" "${${_vtk_scan_module_name}_SPDX_CUSTOM_LICENSE_FILE}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_scan_module_name}_spdx_custom_license_name" "${${_vtk_scan_module_name}_SPDX_CUSTOM_LICENSE_NAME}")

    if (_vtk_scan_ENABLE_TESTS STREQUAL "WANT")
      set_property(GLOBAL
        PROPERTY
          "_vtk_module_${_vtk_scan_module_name}_enable_tests_by_want" "1")
    endif ()
  endforeach ()

  set(_vtk_scan_current_modules "${_vtk_scan_all_modules}")
  vtk_topological_sort(_vtk_scan_all_modules "_vtk_scan_" "_all_depends")

  set(_vtk_scan_provided_modules)
  set(_vtk_scan_required_modules)
  set(_vtk_scan_disabled_modules)

  # Seed the `_vtk_scan_provide_` variables with modules requested and rejected
  # as arguments.
  foreach (_vtk_scan_request_module IN LISTS _vtk_scan_REQUEST_MODULES)
    set("_vtk_scan_provide_${_vtk_scan_request_module}" ON)
    if (DEFINED "_vtk_module_reason_${_vtk_scan_request_module}")
      set("_vtk_scan_provide_reason_${_vtk_scan_request_module}"
        "${_vtk_module_reason_${_vtk_scan_request_module}}")
    else ()
      set("_vtk_scan_provide_reason_${_vtk_scan_request_module}"
        "via `REQUEST_MODULES`")
    endif ()
    _vtk_module_debug(provide "@_vtk_scan_request_module@ is provided ${_vtk_scan_provide_reason_${_vtk_scan_request_module}}")
  endforeach ()
  foreach (_vtk_scan_reject_module IN LISTS _vtk_scan_REJECT_MODULES)
    set("_vtk_scan_provide_${_vtk_scan_reject_module}" OFF)
    if (DEFINED "_vtk_module_reason_${_vtk_scan_reject_module}")
      set("_vtk_scan_provide_reason_${_vtk_scan_reject_module}"
        "${_vtk_module_reason_${_vtk_scan_reject_module}}")
    else ()
      set("_vtk_scan_provide_reason_${_vtk_scan_reject_module}"
        "via `REJECT_MODULES`")
    endif ()
    _vtk_module_debug(provide "@_vtk_scan_reject_module@ is not provided ${_vtk_scan_provide_reason_${_vtk_scan_reject_module}}")
  endforeach ()

  # Traverse the graph classifying the quad-state for enabling modules into a
  # boolean stored in the `_vtk_scan_provide_` variables.
  foreach (_vtk_scan_module IN LISTS _vtk_scan_all_modules)
    if (NOT _vtk_scan_module IN_LIST _vtk_scan_current_modules)
      _vtk_module_debug(provide "@_vtk_scan_module@ is ignored because it is not in the current scan set")
      continue ()
    endif ()

    if (DEFINED "_vtk_scan_provide_${_vtk_scan_module}")
      # Already done.
    elseif (_vtk_scan_enable_${_vtk_scan_module} STREQUAL "YES")
      # Mark enabled modules as to-be-provided. Any errors with requiring a
      # disabled module will be dealt with later.
      set("_vtk_scan_provide_${_vtk_scan_module}" ON)
      set("_vtk_scan_provide_reason_${_vtk_scan_module}"
        "via a `YES` setting (${_vtk_scan_enable_reason_${_vtk_scan_module}})")
      _vtk_module_debug(provide "@_vtk_scan_module@ is provided due to `YES` setting")
    elseif (_vtk_scan_enable_${_vtk_scan_module} STREQUAL "WANT")
      # Check to see if we can provide this module by checking of any of its
      # dependencies have been disabled.
      set(_vtk_scan_test_depends)
      if (NOT ${_vtk_scan_module}_THIRD_PARTY AND _vtk_scan_ENABLE_TESTS STREQUAL "ON")
        # If the tests have to be on, we also need the test dependencies.
        set(_vtk_scan_test_depends "${${_vtk_scan_module}_TEST_DEPENDS}")
      endif ()

      set("_vtk_scan_provide_${_vtk_scan_module}" ON)
      set("_vtk_scan_provide_reason_${_vtk_scan_module}"
        "via a `WANT` setting (${_vtk_scan_enable_reason_${_vtk_scan_module}})")
      _vtk_module_debug(provide "@_vtk_scan_module@ is provided due to `WANT` setting")
      foreach (_vtk_scan_module_depend IN LISTS "${_vtk_scan_module}_DEPENDS" "${_vtk_scan_module}_PRIVATE_DEPENDS" _vtk_scan_test_depends)
        if (DEFINED "_vtk_scan_provide_${_vtk_scan_module_depend}" AND NOT _vtk_scan_provide_${_vtk_scan_module_depend})
          set("_vtk_scan_provide_${_vtk_scan_module}" OFF)
          set("_vtk_scan_provide_reason_${_vtk_scan_module}"
            "due to the ${_vtk_scan_module_depend} module not being available")
          if (DEFINED "_vtk_scan_provide_reason_${_vtk_scan_module_depend}")
            string(APPEND "_vtk_scan_provide_reason_${_vtk_scan_module}"
              " (${_vtk_scan_provide_reason_${_vtk_scan_module_depend}})")
          endif ()
          _vtk_module_debug(provide "@_vtk_scan_module@ is not provided due to not provided dependency @_vtk_scan_module_depend@")
          break ()
        endif ()
      endforeach ()
    elseif (_vtk_scan_enable_${_vtk_scan_module} STREQUAL "DONT_WANT")
      # Check for disabled dependencies and disable if so.
      foreach (_vtk_scan_module_depend IN LISTS "${_vtk_scan_module}_DEPENDS" "${_vtk_scan_module}_PRIVATE_DEPENDS" _vtk_scan_test_depends)
        if (DEFINED "_vtk_scan_provide_${_vtk_scan_module_depend}" AND NOT _vtk_scan_provide_${_vtk_scan_module_depend})
          set("_vtk_scan_provide_${_vtk_scan_module}" OFF)
          set("_vtk_scan_provide_reason_${_vtk_scan_module}"
            "due to the ${_vtk_scan_module_depend} module not being available")
          if (DEFINED "_vtk_scan_provide_reason_${_vtk_scan_module_depend}")
            string(APPEND "_vtk_scan_provide_reason_${_vtk_scan_module}"
              " (${_vtk_scan_provide_reason_${_vtk_scan_module_depend}})")
          endif ()
          _vtk_module_debug(provide "@_vtk_scan_module@ is not provided due to not provided dependency @_vtk_scan_module_depend@")
          break ()
        endif ()
      endforeach ()
    elseif (_vtk_scan_enable_${_vtk_scan_module} STREQUAL "NO")
      # Disable the module.
      set("_vtk_scan_provide_${_vtk_scan_module}" OFF)
      set("_vtk_scan_provide_reason_${_vtk_scan_module}"
        "via a `NO` setting (${_vtk_scan_enable_reason_${_vtk_scan_module}})")
      _vtk_module_debug(provide "@_vtk_scan_module@ is not provided due to `NO` setting")
    endif ()

    # Collect disabled modules into a list.
    if (DEFINED "_vtk_scan_provide_${_vtk_scan_module}" AND NOT _vtk_scan_provide_${_vtk_scan_module})
      list(APPEND _vtk_scan_disabled_modules
        "${_vtk_scan_module}")
    endif ()

    if (NOT DEFINED "_vtk_scan_provide_${_vtk_scan_module}")
      _vtk_module_debug(provide "@_vtk_scan_module@ is indeterminate (${_vtk_scan_enable_${_vtk_scan_module}})")
    endif ()
  endforeach ()

  # Scan all modules from the top of tree to the bottom.
  list(REVERSE _vtk_scan_all_modules)
  foreach (_vtk_scan_module IN LISTS _vtk_scan_all_modules)
    if (NOT _vtk_scan_module IN_LIST _vtk_scan_current_modules)
      continue ()
    endif ()

    # If we're providing this module...
    if (_vtk_scan_provide_${_vtk_scan_module})
      list(APPEND _vtk_scan_provided_modules
        "${_vtk_scan_module}")

      # Grab any test dependencies that are required.
      set(_vtk_scan_test_depends)
      set(_vtk_scan_test_wants)
      if (_vtk_scan_ENABLE_TESTS STREQUAL "ON")
        set_property(GLOBAL APPEND
          PROPERTY
            "_vtk_module_test_modules" "${_vtk_scan_module}")
        set(_vtk_scan_test_depends "${${_vtk_scan_module}_TEST_DEPENDS}")
      elseif (_vtk_scan_ENABLE_TESTS STREQUAL "WANT")
        set_property(GLOBAL APPEND
          PROPERTY
            "_vtk_module_test_modules" "${_vtk_scan_module}")
        set(_vtk_scan_test_wants _vtk_scan_wants_marker ${${_vtk_scan_module}_TEST_DEPENDS})
      elseif (_vtk_scan_ENABLE_TESTS STREQUAL "DEFAULT")
        set_property(GLOBAL APPEND
          PROPERTY
            "_vtk_module_test_modules" "${_vtk_scan_module}")
      elseif (_vtk_scan_ENABLE_TESTS STREQUAL "OFF")
        # Nothing to do.
      else ()
        message(FATAL_ERROR
          "Unrecognized option for ENABLE_TESTS: ${_vtk_module_ENABLE_TESTS}.")
      endif ()

      # Add all dependent modules to the list of required or provided modules.
      set(_vtk_scan_is_wanting 0)
      foreach (_vtk_scan_module_depend IN LISTS "${_vtk_scan_module}_DEPENDS" "${_vtk_scan_module}_PRIVATE_DEPENDS" _vtk_scan_test_depends _vtk_scan_test_wants)
        if (_vtk_scan_module_depend STREQUAL "_vtk_scan_wants_marker")
          set(_vtk_scan_is_wanting 1)
          continue ()
        endif ()
        # Though we need to error if this would cause a disabled module to be
        # provided.
        if (_vtk_scan_module_depend IN_LIST _vtk_scan_disabled_modules)
          if (_vtk_scan_is_wanting)
            continue ()
          else ()
            message(FATAL_ERROR
              "The ${_vtk_scan_module} module (enabled "
              "${_vtk_scan_provide_reason_${_vtk_scan_module}}) requires the "
              "disabled module ${_vtk_scan_module_depend} (disabled "
              "${_vtk_scan_provide_reason_${_vtk_scan_module_depend}}).")
          endif ()
        endif ()

        if (DEFINED "_vtk_scan_provide_${_vtk_scan_module_depend}")
          if (NOT _vtk_scan_provide_${_vtk_scan_module_depend})
            message(FATAL_ERROR
              "The ${_vtk_scan_module_depend} module (disabled "
              "${_vtk_scan_provide_reason_${_vtk_scan_module_depend}}) should "
              "be provided because it is required by ${_vtk_scan_module} "
              "(${_vtk_scan_provide_reason_${_vtk_scan_module}})")
          endif ()
          continue ()
        endif ()
        set("_vtk_scan_provide_reason_${_vtk_scan_module_depend}"
          "via dependency from ${_vtk_scan_module}")
        if (DEFINED "_vtk_scan_provide_reason_${_vtk_scan_module}")
          string(APPEND "_vtk_scan_provide_reason_${_vtk_scan_module_depend}"
            " (${_vtk_scan_provide_reason_${_vtk_scan_module}})")
        endif ()
        set("_vtk_scan_provide_${_vtk_scan_module_depend}" ON)

        if (NOT _vtk_scan_module_depend IN_LIST _vtk_scan_current_modules)
          if (NOT TARGET "${_vtk_scan_module_depend}")
            _vtk_module_debug(provide "@_vtk_scan_module_depend@ is external and required due to dependency from @_vtk_scan_module@")
          endif ()
          list(APPEND _vtk_scan_required_modules
            "${_vtk_scan_module_depend}")
        else ()
          _vtk_module_debug(provide "@_vtk_scan_module_depend@ is provided due to dependency from @_vtk_scan_module@")
          list(APPEND _vtk_scan_provided_modules
            "${_vtk_scan_module_depend}")
        endif ()
      endforeach ()
    endif ()
  endforeach ()

  if (_vtk_scan_provided_modules)
    list(REMOVE_DUPLICATES _vtk_scan_provided_modules)
  endif ()

  set(_vtk_scan_provided_kits)

  # Build a list of kits which contain the provided modules.
  foreach (_vtk_scan_provided_module IN LISTS _vtk_scan_provided_modules)
    if (${_vtk_scan_provided_module}_KIT)
      list(APPEND _vtk_scan_provided_kits
        "${${_vtk_scan_provided_module}_KIT}")
      set_property(GLOBAL APPEND
        PROPERTY
          "_vtk_kit_${${_vtk_scan_provided_module}_KIT}_kit_modules" "${_vtk_scan_provided_module}")
    endif ()
  endforeach ()

  if (_vtk_scan_provided_kits)
    list(REMOVE_DUPLICATES _vtk_scan_provided_kits)
  endif ()

  if (_vtk_scan_required_modules)
    list(REMOVE_DUPLICATES _vtk_scan_required_modules)
  endif ()

  set(_vtk_scan_unrecognized_modules
    ${_vtk_scan_REQUEST_MODULES}
    ${_vtk_scan_REJECT_MODULES})

  if (_vtk_scan_unrecognized_modules AND (_vtk_scan_provided_modules OR _vtk_scan_rejected_modules))
    list(REMOVE_ITEM _vtk_scan_unrecognized_modules
      ${_vtk_scan_provided_modules}
      ${_vtk_scan_rejected_modules})
  endif ()

  set("${_vtk_scan_PROVIDES_MODULES}"
    ${_vtk_scan_provided_modules}
    PARENT_SCOPE)

  if (DEFINED _vtk_scan_REQUIRES_MODULES)
    set("${_vtk_scan_REQUIRES_MODULES}"
      ${_vtk_scan_required_modules}
      PARENT_SCOPE)
  endif ()

  if (DEFINED _vtk_scan_UNRECOGNIZED_MODULES)
    set("${_vtk_scan_UNRECOGNIZED_MODULES}"
      ${_vtk_scan_unrecognized_modules}
      PARENT_SCOPE)
  endif ()

  if (DEFINED _vtk_scan_PROVIDES_KITS)
    set("${_vtk_scan_PROVIDES_KITS}"
      ${_vtk_scan_provided_kits}
      PARENT_SCOPE)
  endif ()
endfunction ()

#[==[.rst:
.. _module-target-functions:

Module-as-target functions
==========================

Due to the nature of VTK modules supporting being built as kits, the module
name might not be usable as a target to CMake's `target_` family of commands.
Instead, there are various wrappers around them which take the module name as
an argument. These handle the forwarding of relevant information to the kit
library as well where necessary.

* :cmake:command:`vtk_module_set_properties`
* :cmake:command:`vtk_module_set_property`
* :cmake:command:`vtk_module_get_property`
* :cmake:command:`vtk_module_depend`
* :cmake:command:`vtk_module_include`
* :cmake:command:`vtk_module_definitions`
* :cmake:command:`vtk_module_compile_options`
* :cmake:command:`vtk_module_compile_features`
* :cmake:command:`vtk_module_link`
* :cmake:command:`vtk_module_link_options`
#]==]

#[==[.rst:

.. _module-target-internals:

Module target internals
=======================

When manipulating modules as targets, there are a few functions provided for
use in wrapping code to more easily access them.

* :cmake:command:`_vtk_module_real_target`
* :cmake:command:`_vtk_module_real_target_kit`
#]==]

#[==[.rst:
  .. cmake:command:: _vtk_module_real_target

  The real target for a module
  |module-internal|

  .. code-block:: cmake

    _vtk_module_real_target(<var> <module>)

  Sometimes the actual, core target for a module is required (e.g., setting
  CMake-level target properties or install rules). This function returns the real
  target for a module.
#]==]
function (_vtk_module_real_target var module)
  if (ARGN)
    message(FATAL_ERROR
      "Unparsed arguments for _vtk_module_real_target: ${ARGN}.")
  endif ()

  set(_vtk_real_target_res "")
  if (TARGET "${module}")
    get_property(_vtk_real_target_imported
      TARGET    "${module}"
      PROPERTY  IMPORTED)
    if (_vtk_real_target_imported)
      set(_vtk_real_target_res "${module}")
    endif ()
  endif ()

  if (NOT _vtk_real_target_res)
    get_property(_vtk_real_target_res GLOBAL
      PROPERTY "_vtk_module_${module}_target_name")
    # Querying during the build.
    if (DEFINED _vtk_build_BUILD_WITH_KITS AND _vtk_build_BUILD_WITH_KITS)
      get_property(_vtk_real_target_kit GLOBAL
        PROPERTY "_vtk_module_${module}_kit")
      if (_vtk_real_target_kit)
        string(APPEND _vtk_real_target_res "-objects")
      endif ()
    # A query for after the module is built.
    elseif (TARGET "${_vtk_real_target_res}-objects")
      string(APPEND _vtk_real_target_res "-objects")
    endif ()
  endif ()

  if (NOT _vtk_real_target_res)
    set(_vtk_real_target_msg "")
    if (NOT TARGET "${module}")
      if (DEFINED _vtk_build_module)
        set(_vtk_real_target_msg
          " Is a module dependency missing?")
      elseif (TARGET "${module}")
        set(_vtk_real_target_msg
          " It's a target, but is it a VTK module?")
      else ()
        set(_vtk_real_target_msg
          " The module name is not a CMake target. Is there a typo? Is it missing a `Package::` prefix? Is a `find_package` missing a required component?")
      endif ()
    endif ()
    message(FATAL_ERROR
      "Failed to determine the real target for the `${module}` "
      "module.${_vtk_real_target_msg}")
  endif ()

  set("${var}"
    "${_vtk_real_target_res}"
    PARENT_SCOPE)
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_real_target_kit

  The real target for a kit |module-internal|

  .. code-block:: cmake

    _vtk_module_real_target_kit(<var> <kit>)

  Sometimes the actual, core target for a module is required (e.g., setting
  CMake-level target properties or install rules). This function returns the real
  target for a kit.
#]==]
function (_vtk_module_real_target_kit var kit)
  if (ARGN)
    message(FATAL_ERROR
      "Unparsed arguments for _vtk_module_real_target_kit: ${ARGN}.")
  endif ()

  set(_vtk_real_target_res "")
  if (TARGET "${kit}")
    get_property(_vtk_real_target_imported
      TARGET    "${kit}"
      PROPERTY  IMPORTED)
    if (_vtk_real_target_imported)
      set(_vtk_real_target_res "${kit}")
    endif ()
  endif ()

  if (NOT _vtk_real_target_res)
    get_property(_vtk_real_target_res GLOBAL
      PROPERTY "_vtk_kit_${kit}_target_name")
  endif ()

  if (NOT _vtk_real_target_res)
    message(FATAL_ERROR
      "Failed to determine the real target for the `${kit}` kit.")
  endif ()

  set("${var}"
    "${_vtk_real_target_res}"
    PARENT_SCOPE)
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_set_properties

  Set multiple properties on a module
  |module|

  A wrapper around `set_target_properties` that works for modules.

  .. code-block:: cmake

    vtk_module_set_properties(<module>
      [<property> <value>]...)
#]==]
function (vtk_module_set_properties module)
  _vtk_module_real_target(_vtk_set_properties_target "${module}")

  set_target_properties("${_vtk_set_properties_target}"
    PROPERTIES
      ${ARGN})
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_set_property

  Set a property on a module. |module|

  A wrapper around ``set_property(TARGET)`` that works for modules.

  .. code-block:: cmake

    vtk_module_set_property(<module>
      [APPEND] [APPEND_STRING]
      PROPERTY  <property>
      VALUE     <value>...)
#]==]
function (vtk_module_set_property module)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_property
    "APPEND;APPEND_STRING"
    "PROPERTY"
    "VALUE")

  if (_vtk_property_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_set_property: "
      "${_vtk_property_UNPARSED_ARGUMENTS}.")
  endif ()

  if (NOT DEFINED _vtk_property_PROPERTY)
    message(FATAL_ERROR
      "The `PROPERTY` argument is required.")
  endif ()

  if (NOT DEFINED _vtk_property_VALUE)
    message(FATAL_ERROR
      "The `VALUE` argument is required.")
  endif ()

  if (_vtk_property_APPEND AND _vtk_property_APPEND_STRING)
    message(FATAL_ERROR
      "`APPEND` and `APPEND_STRING` may not be used at the same time.")
  endif ()

  set(_vtk_property_args)
  if (_vtk_property_APPEND)
    list(APPEND _vtk_property_args
      APPEND)
  endif ()
  if (_vtk_property_APPEND_STRING)
    list(APPEND _vtk_property_args
      APPEND_STRING)
  endif ()

  _vtk_module_real_target(_vtk_property_target "${module}")

  set_property(TARGET "${_vtk_property_target}"
    ${_vtk_property_args}
    PROPERTY
      "${_vtk_property_PROPERTY}" "${_vtk_property_VALUE}")
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_get_property

  Get a property from a module
  |module|

  A wrapper around `get_property(TARGET)` that works for modules.

  .. code-block:: cmake

    vtk_module_get_property(<module>
      PROPERTY  <property>
      VARIABLE  <variable>)

  The variable name passed to the ``VARIABLE`` argument will be unset if the
  property is not set (rather than the empty string).
#]==]
function (vtk_module_get_property module)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_property
    ""
    "PROPERTY;VARIABLE"
    "")

  if (_vtk_property_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_get_property: "
      "${_vtk_property_UNPARSED_ARGUMENTS}.")
  endif ()

  if (NOT DEFINED _vtk_property_PROPERTY)
    message(FATAL_ERROR
      "The `PROPERTY` argument is required.")
  endif ()

  if (NOT DEFINED _vtk_property_VARIABLE)
    message(FATAL_ERROR
      "The `VARIABLE` argument is required.")
  endif ()

  _vtk_module_real_target(_vtk_property_target "${module}")

  get_property(_vtk_property_is_set
    TARGET    "${_vtk_property_target}"
    PROPERTY  "${_vtk_property_PROPERTY}"
    SET)
  if (_vtk_property_is_set)
    get_property(_vtk_property_value
      TARGET    "${_vtk_property_target}"
      PROPERTY  "${_vtk_property_PROPERTY}")

    set("${_vtk_property_VARIABLE}"
      "${_vtk_property_value}"
      PARENT_SCOPE)
  else ()
    unset("${_vtk_property_VARIABLE}"
      PARENT_SCOPE)
  endif ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_target_function

  Generate arguments for target function wrappers |module-impl|

  Create the ``INTERFACE``, ``PUBLIC``, and ``PRIVATE`` arguments for a function
  wrapping CMake's ``target_`` functions to call the wrapped function.

  This is necessary because not all of the functions support empty lists given a
  keyword.
#]==]
function (_vtk_module_target_function prefix)
  foreach (visibility IN ITEMS INTERFACE PUBLIC PRIVATE)
    if (${prefix}_${visibility})
      set("${prefix}_${visibility}_args"
        "${visibility}"
        ${${prefix}_${visibility}}
        PARENT_SCOPE)
    endif ()
  endforeach ()
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_depend

  Add dependencies to a module |module|

  A wrapper around ``add_dependencies`` that works for modules.

  .. code-block:: cmake

    vtk_module_depend(<module> <depend>...)

#]==]
function (vtk_module_depend module)
  _vtk_module_real_target(_vtk_depend_target "${module}")

  add_dependencies("${_vtk_depend_target}"
    ${ARGN})
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_sources

  Add source files to a module. |module|

  A wrapper around `target_sources` that works for modules.

  .. code-block:: cmake

    vtk_module_sources(<module>
      [PUBLIC     <source>...]
      [PRIVATE    <source>...]
      [INTERFACE  <source>...])
#]==]
function (vtk_module_sources module)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_sources
    ""
    ""
    "INTERFACE;PUBLIC;PRIVATE")

  if (_vtk_sources_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_sources: "
      "${_vtk_sources_UNPARSED_ARGUMENTS}.")
  endif ()

  _vtk_module_real_target(_vtk_sources_target "${module}")
  _vtk_module_target_function(_vtk_sources)

  if (NOT _vtk_sources_INTERFACE_args AND
      NOT _vtk_sources_PUBLIC_args AND
      NOT _vtk_sources_PRIVATE_args)
    return ()
  endif ()

  target_sources("${_vtk_sources_target}"
    ${_vtk_sources_INTERFACE_args}
    ${_vtk_sources_PUBLIC_args}
    ${_vtk_sources_PRIVATE_args})
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_include

 Add include directories to a module
 |module|

 A wrapper around `target_include_directories` that works for modules.

 .. code-block:: cmake

   vtk_module_include(<module>
     [SYSTEM]
     [PUBLIC     <directory>...]
     [PRIVATE    <directory>...]
     [INTERFACE  <directory>...])
#]==]
function (vtk_module_include module)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_include
    "SYSTEM"
    ""
    "INTERFACE;PUBLIC;PRIVATE")

  if (_vtk_include_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_include: "
      "${_vtk_include_UNPARSED_ARGUMENTS}.")
  endif ()

  _vtk_module_real_target(_vtk_include_target "${module}")
  _vtk_module_target_function(_vtk_include)

  set(_vtk_include_system_arg)
  if (_vtk_include_SYSTEM)
    set(_vtk_include_system_arg SYSTEM)
  endif ()

  if (NOT _vtk_include_INTERFACE_args AND
      NOT _vtk_include_PUBLIC_args AND
      NOT _vtk_include_PRIVATE_args)
    return ()
  endif ()

  target_include_directories("${_vtk_include_target}"
    ${_vtk_include_system_arg}
    ${_vtk_include_INTERFACE_args}
    ${_vtk_include_PUBLIC_args}
    ${_vtk_include_PRIVATE_args})
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_definitions

  Add compile definitions to a module. |module|

  A wrapper around ``target_compile_definitions`` that works for modules.

  .. code-block:: cmake

    vtk_module_definitions(<module>
      [PUBLIC     <define>...]
      [PRIVATE    <define>...]
      [INTERFACE  <define>...])

#]==]
function (vtk_module_definitions module)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_definitions
    ""
    ""
    "INTERFACE;PUBLIC;PRIVATE")

  if (_vtk_definitions_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_definitions: "
      "${_vtk_definitions_UNPARSED_ARGUMENTS}.")
  endif ()

  _vtk_module_real_target(_vtk_definitions_target "${module}")
  _vtk_module_target_function(_vtk_definitions)

  if (NOT _vtk_definitions_INTERFACE_args AND
      NOT _vtk_definitions_PUBLIC_args AND
      NOT _vtk_definitions_PRIVATE_args)
    return ()
  endif ()

  target_compile_definitions("${_vtk_definitions_target}"
    ${_vtk_definitions_INTERFACE_args}
    ${_vtk_definitions_PUBLIC_args}
    ${_vtk_definitions_PRIVATE_args})
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_compile_options

  Add compile options to a module. |module|

  A wrapper around ``target_compile_options`` that works for modules.

  .. code-block:: cmake

    vtk_module_compile_options(<module>
      [PUBLIC     <option>...]
      [PRIVATE    <option>...]
      [INTERFACE  <option>...])
#]==]
function (vtk_module_compile_options module)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_compile_options
    ""
    ""
    "INTERFACE;PUBLIC;PRIVATE")

  if (_vtk_compile_options_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_compile_options: "
      "${_vtk_compile_options_UNPARSED_ARGUMENTS}.")
  endif ()

  _vtk_module_real_target(_vtk_compile_options_target "${module}")
  _vtk_module_target_function(_vtk_compile_options)

  if (NOT _vtk_compile_options_INTERFACE_args AND
      NOT _vtk_compile_options_PUBLIC_args AND
      NOT _vtk_compile_options_PRIVATE_args)
    return ()
  endif ()

  target_compile_options("${_vtk_compile_options_target}"
    ${_vtk_compile_options_INTERFACE_args}
    ${_vtk_compile_options_PUBLIC_args}
    ${_vtk_compile_options_PRIVATE_args})
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_compile_features

  Add compile features to a module. |module|

  A wrapper around `target_compile_features` that works for modules.

  .. code-block:: cmake

    vtk_module_compile_features(<module>
      [PUBLIC     <feature>...]
      [PRIVATE    <feature>...]
      [INTERFACE  <feature>...])
#]==]
function (vtk_module_compile_features module)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_compile_features
    ""
    ""
    "INTERFACE;PUBLIC;PRIVATE")

  if (_vtk_compile_features_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_compile_features: "
      "${_vtk_compile_features_UNPARSED_ARGUMENTS}.")
  endif ()

  _vtk_module_real_target(_vtk_compile_features_target "${module}")
  _vtk_module_target_function(_vtk_compile_features)

  if (NOT _vtk_compile_features_INTERFACE_args AND
      NOT _vtk_compile_features_PUBLIC_args AND
      NOT _vtk_compile_features_PRIVATE_args)
    return ()
  endif ()

  target_compile_features("${_vtk_compile_features_target}"
    ${_vtk_compile_features_INTERFACE_args}
    ${_vtk_compile_features_PUBLIC_args}
    ${_vtk_compile_features_PRIVATE_args})
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_private_kit_link_target

  Manage the private link target for a module. |module-impl|

  This function manages the private link target for a module.

  .. code-block:: cmake

    _vtk_private_kit_link_target(<module>
      [CREATE_IF_NEEDED]
      [SETUP_TARGET_NAME <var>]
      [USAGE_TARGET_NAME <var>])
#]==]
function (_vtk_private_kit_link_target module)
  cmake_parse_arguments(_vtk_private_kit_link_target
    "CREATE_IF_NEEDED"
    "SETUP_TARGET_NAME;USAGE_TARGET_NAME"
    ""
    ${ARGN})

  if (_vtk_private_kit_link_target_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for _vtk_private_kit_link_target: "
      "${_vtk_private_kit_link_target_UNPARSED_ARGUMENTS}.")
  endif ()

  # Compute the target name.
  get_property(_vtk_private_kit_link_base_target_name GLOBAL
    PROPERTY "_vtk_module_${module}_target_name")
  if (NOT _vtk_private_kit_link_base_target_name)
    message(FATAL_ERROR
      "_vtk_private_kit_link_target only works for modules built in the "
      "current project.")
  endif ()

  set(_vtk_private_kit_link_target_setup_name
    "${_vtk_private_kit_link_base_target_name}-private-kit-links")
  get_property(_vtk_private_kit_link_namespace GLOBAL
    PROPERTY "_vtk_module_${module}_namespace")
  if (_vtk_private_kit_link_namespace)
    set(_vtk_private_kit_link_target_usage_name
      "${_vtk_private_kit_link_namespace}::${_vtk_private_kit_link_target_setup_name}")
  else ()
    set(_vtk_private_kit_link_target_usage_name
      ":${_vtk_private_kit_link_target_setup_name}")
  endif ()

  # Create the target if requested.
  if (_vtk_private_kit_link_target_CREATE_IF_NEEDED AND
      NOT TARGET "${_vtk_private_kit_link_target_setup_name}")
    add_library("${_vtk_private_kit_link_target_setup_name}" INTERFACE)
    if (NOT _vtk_private_kit_link_target_setup_name STREQUAL _vtk_private_kit_link_target_usage_name)
      add_library("${_vtk_private_kit_link_target_usage_name}" ALIAS
        "${_vtk_private_kit_link_target_setup_name}")
    endif ()
    _vtk_module_install("${_vtk_private_kit_link_target_setup_name}")
  endif ()

  if (_vtk_private_kit_link_target_SETUP_TARGET_NAME)
    set("${_vtk_private_kit_link_target_SETUP_TARGET_NAME}"
      "${_vtk_private_kit_link_target_setup_name}"
      PARENT_SCOPE)
  endif ()

  if (_vtk_private_kit_link_target_USAGE_TARGET_NAME)
    set("${_vtk_private_kit_link_target_USAGE_TARGET_NAME}"
      "${_vtk_private_kit_link_target_usage_name}"
      PARENT_SCOPE)
  endif ()
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_link

  Add link libraries to a module. |module|

  A wrapper around `target_link_libraries` that works for modules. Note that this
  function does extra work in kit builds, so circumventing it may break in kit
  builds.

  The ``NO_KIT_EXPORT_IF_SHARED`` argument may be passed to additionally prevent
  leaking ``PRIVATE`` link targets from kit builds. Intended to be used for
  targets coming from a ``vtk_module_find_package(PRIVATE_IF_SHARED)`` call.
  Applies to all ``PRIVATE`` arguments; if different treatment is needed for
  subsets of these arguments, use a separate call to ``vtk_module_link``.

  .. code-block:: cmake

    vtk_module_link(<module>
      [NO_KIT_EXPORT_IF_SHARED]
      [PUBLIC     <link item>...]
      [PRIVATE    <link item>...]
      [INTERFACE  <link item>...])
#]==]
function (vtk_module_link module)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_link
    "NO_KIT_EXPORT_IF_SHARED"
    ""
    "INTERFACE;PUBLIC;PRIVATE")

  if (_vtk_link_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_link: "
      "${_vtk_link_UNPARSED_ARGUMENTS}.")
  endif ()

  _vtk_module_real_target(_vtk_link_target "${module}")
  _vtk_module_target_function(_vtk_link)

  get_property(_vtk_link_kit GLOBAL
    PROPERTY "_vtk_module_${module}_kit")
  if (_vtk_link_kit)
    if (_vtk_link_PRIVATE)
      set(_vtk_link_PRIVATE_seed_args "${_vtk_link_PRIVATE_args}")
      set(_vtk_link_PRIVATE_args "PRIVATE")

      _vtk_private_kit_link_target("${module}"
        CREATE_IF_NEEDED
        SETUP_TARGET_NAME _vtk_link_private_kit_link_target)
      foreach (_vtk_link_private IN LISTS _vtk_link_PRIVATE)
        set(_vtk_link_private_for_kit
          "$<LINK_ONLY:${_vtk_link_private}>")
        if (_vtk_link_NO_KIT_EXPORT_IF_SHARED AND BUILD_SHARED_LIBS)
          set(_vtk_link_private_for_kit
            "$<BUILD_INTERFACE:${_vtk_link_private_for_kit}>")
          set(_vtk_link_private
            "$<BUILD_INTERFACE:${_vtk_link_private}>")
        endif ()
        target_link_libraries("${_vtk_link_private_kit_link_target}"
          INTERFACE
            "${_vtk_link_private_for_kit}")
        list(APPEND _vtk_link_PRIVATE_args
          "${_vtk_link_private}")
      endforeach ()
    endif ()
  endif ()

  if (NOT _vtk_link_INTERFACE_args AND
      NOT _vtk_link_PUBLIC_args AND
      NOT _vtk_link_PRIVATE_args)
    return ()
  endif ()

  target_link_libraries("${_vtk_link_target}"
    ${_vtk_link_INTERFACE_args}
    ${_vtk_link_PUBLIC_args}
    ${_vtk_link_PRIVATE_args})
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_link_options

  Add link options to a module. |module|

  A wrapper around `target_link_options` that works for modules.

  .. code-block:: cmake

    vtk_module_link_options(<module>
      [PUBLIC     <option>...]
      [PRIVATE    <option>...]
      [INTERFACE  <option>...])
#]==]
function (vtk_module_link_options module)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_link_options
    ""
    ""
    "INTERFACE;PUBLIC;PRIVATE")

  if (_vtk_link_options_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_link_options: "
      "${_vtk_link_options_UNPARSED_ARGUMENTS}.")
  endif ()

  _vtk_module_real_target(_vtk_link_options_target "${module}")
  _vtk_module_target_function(_vtk_link_options)

  if (NOT _vtk_link_options_INTERFACE_args AND
      NOT _vtk_link_options_PUBLIC_args AND
      NOT _vtk_link_options_PRIVATE_args)
    return ()
  endif ()

  target_link_options("${_vtk_link_options_target}"
    ${_vtk_link_options_INTERFACE_args}
    ${_vtk_link_options_PUBLIC_args}
    ${_vtk_link_options_PRIVATE_args})
endfunction ()

#[==[.rst:
.. _module-properties:

Module properties
=================
|module-internal|

The VTK module system leverages CMake's target propagation and storage. As
such, there are a number of properties added to the targets representing
modules. These properties are intended for use by the module system and
associated functionality. In particular, more properties may be available by
language wrappers.

.. _module-properties-naming:

Naming properties
^^^^^^^^^^^^^^^^^


When creating properties for use with the module system, they should be
prefixed with ``INTERFACE_vtk_module_``. The ``INTERFACE_`` portion is required in
order to work with interface libraries. The ``vtk_module_`` portion is to avoid
colliding with any other properties. This function assumes this naming scheme
for some of its convenience features as well.

Properties should be the same in the local build as well as when imported to
ease use.

.. _module-properties-system:

VTK module system properties
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There are a number of properties that are used and expected by the core of the
module system. These are generally module metadata (module dependencies,
whether to wrap or not, etc.). The properties all have the
``INTERFACE_vtk_module_`` prefix mentioned in the previous section.

* ``third_party``: If set, the module represents a third party
  dependency and should be treated specially. Third party modules are very
  restricted and generally do not have any other properties set on them.
* ``exclude_wrap``: If set, the module should not be wrapped by an external
  language.
* ``depends``: The list of dependent modules. Language wrappers will generally
  require this to satisfy references to parent classes of the classes in the
  module.
* ``private_depends``: The list of privately dependent modules. Language
  wrappers may require this to satisfy references to parent classes of the
  classes in the module.
* ``optional_depends``: The list of optionally dependent modules. Language
  wrappers may require this to satisfy references to parent classes of the
  classes in the module.
* ``kit``: The kit the module is a member of. Only set if the module is
  actually a member of the kit (i.e., the module was built with
  ``BUILD_WITH_KITS ON``).
* ``implements``: The list of modules for which this module registers to. This
  is used by the autoinit subsystem and generally is not required.
* ``implementable``: If set, this module provides registries which may be
  populated by dependent modules. It is used to check the ``implements``
  property to help minimize unnecessary work from the autoinit subsystem.
* ``needs_autoinit``: If set, linking to this module requires the autoinit
  subsystem to ensure that registries in modules are fully populated.
* ``headers``: Paths to the public headers from the module. These are the
  headers which should be handled by language wrappers.
* ``hierarchy``: The path to the hierarchy file describing inheritance of the
  classes for use in language wrappers.
* ``forward_link``: Usage requirements that must be forwarded even though the
    module is linked to privately.
* ``include_marshal``: If set, the whole module opts into automatic code generation
  of (de)serializers. Note that only classes annotated with VTK_MARSHALAUTO are
  considered for code generation.

Kits have the following properties available (but only if kits are enabled):

* ``kit_modules``: Modules which are compiled into the kit.
#]==]

#[==[.rst:
.. cmake:command:: _vtk_module_set_module_property

  Set a module property. |module-internal|

  This function sets a :ref:`module property <module-properties>` on a module. The
  required prefix will automatically be added to the passed name.

  .. code-block:: cmake

    _vtk_module_set_module_property(<module>
      [APPEND] [APPEND_STRING]
      PROPERTY  <property>
      VALUE     <value>...)
#]==]
function (_vtk_module_set_module_property module)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_property
    "APPEND;APPEND_STRING"
    "PROPERTY"
    "VALUE")

  if (_vtk_property_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_set_module_property: "
      "${_vtk_property_UNPARSED_ARGUMENTS}.")
  endif ()

  if (NOT DEFINED _vtk_property_PROPERTY)
    message(FATAL_ERROR
      "The `PROPERTY` argument is required.")
  endif ()

  if (NOT DEFINED _vtk_property_VALUE)
    message(FATAL_ERROR
      "The `VALUE` argument is required.")
  endif ()

  if (_vtk_property_APPEND AND _vtk_property_APPEND_STRING)
    message(FATAL_ERROR
      "`APPEND` and `APPEND_STRING` may not be used at the same time.")
  endif ()

  set(_vtk_property_args)
  if (_vtk_property_APPEND)
    list(APPEND _vtk_property_args
      APPEND)
  endif ()
  if (_vtk_property_APPEND_STRING)
    list(APPEND _vtk_property_args
      APPEND_STRING)
  endif ()

  get_property(_vtk_property_is_alias
    TARGET    "${module}"
    PROPERTY  ALIASED_TARGET
    SET)
  if (_vtk_property_is_alias)
    _vtk_module_real_target(_vtk_property_target "${module}")
  else ()
    set(_vtk_property_target "${module}")
  endif ()

  set_property(TARGET "${_vtk_property_target}"
    ${_vtk_property_args}
    PROPERTY
      "INTERFACE_vtk_module_${_vtk_property_PROPERTY}" "${_vtk_property_VALUE}")
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_get_module_property

  Get a module property.  |module-internal|

  Get a :ref:`module property <module-properties>` from a module.

  .. code-block:: cmake

    _vtk_module_get_module_property(<module>
      PROPERTY  <property>
      VARIABLE  <variable>)


As with :cmake:command:`vtk_module_get_property`, the output variable will be unset if the
property is not set. The property name is automatically prepended with the
required prefix.
#]==]
function (_vtk_module_get_module_property module)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_property
    ""
    "PROPERTY;VARIABLE"
    "")

  if (_vtk_property_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_get_module_property: "
      "${_vtk_property_UNPARSED_ARGUMENTS}.")
  endif ()

  if (NOT DEFINED _vtk_property_PROPERTY)
    message(FATAL_ERROR
      "The `PROPERTY` argument is required.")
  endif ()

  if (NOT DEFINED _vtk_property_VARIABLE)
    message(FATAL_ERROR
      "The `VARIABLE` argument is required.")
  endif ()

  get_property(_vtk_property_is_alias
    TARGET    "${module}"
    PROPERTY  ALIASED_TARGET
    SET)
  if (_vtk_property_is_alias)
    _vtk_module_real_target(_vtk_property_target "${module}")
  else ()
    set(_vtk_property_target "${module}")
  endif ()

  get_property(_vtk_property_is_set
    TARGET    "${_vtk_property_target}"
    PROPERTY  "INTERFACE_vtk_module_${_vtk_property_PROPERTY}"
    SET)
  if (_vtk_property_is_set)
    get_property(_vtk_property_value
      TARGET    "${_vtk_property_target}"
      PROPERTY  "INTERFACE_vtk_module_${_vtk_property_PROPERTY}")

    set("${_vtk_property_VARIABLE}"
      "${_vtk_property_value}"
      PARENT_SCOPE)
  else ()
    unset("${_vtk_property_VARIABLE}"
      PARENT_SCOPE)
  endif ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_check_destinations

  Check that destinations are valid. |module-internal|

  All installation destinations are expected to be relative so that
  ``CMAKE_INSTALL_PREFIX`` can be relied upon in all code paths. This function may
  be used to verify that destinations are relative.

  .. code-block:: cmake

    _vtk_module_check_destinations(<prefix> [<suffix>...])

  For each ``suffix``, ``prefix`` is prefixed to it and the resulting variable name
  is checked for validity as an install prefix. Raises an error if any is
  invalid.
#]==]
function (_vtk_module_check_destinations prefix)
  foreach (suffix IN LISTS ARGN)
    if (IS_ABSOLUTE "${${prefix}${suffix}}")
      message(FATAL_ERROR
        "The `${suffix}` must not be an absolute path. Use "
        "`CMAKE_INSTALL_PREFIX` to keep everything in a single installation "
        "prefix.")
    endif ()
  endforeach ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_write_import_prefix

  Write an import prefix statement. |module-internal|

  CMake files, once installed, may need to construct paths to other locations
  within the install prefix. This function writes a prefix computation for file
  given its install destination.

  .. code-block:: cmake

    _vtk_module_write_import_prefix(<file> <destination>)

  The passed file is cleared so that it occurs at the top of the file. The prefix
  is available in the file as the ``_vtk_module_import_prefix`` variable. It is
  recommended to unset the variable at the end of the file.
#]==]
function (_vtk_module_write_import_prefix file destination)
  if (IS_ABSOLUTE "${destination}")
    message(FATAL_ERROR
      "An import prefix cannot be determined from an absolute installation "
      "destination. Use `CMAKE_INSTALL_PREFIX` to keep everything in a single "
      "installation prefix.")
  endif ()

  file(WRITE "${file}"
    "set(_vtk_module_import_prefix \"\${CMAKE_CURRENT_LIST_DIR}\")\n")
  while (destination)
    get_filename_component(destination "${destination}" DIRECTORY)
    file(APPEND "${file}"
      "get_filename_component(_vtk_module_import_prefix \"\${_vtk_module_import_prefix}\" DIRECTORY)\n")
  endwhile ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_export_properties

  Export properties on modules and targets. |module-internal|

  This function is intended for use in support functions which leverage the
  module system, not by general system users. This function supports exporting
  properties from the build into dependencies via target properties which are
  loaded from a project's config file which is loaded via CMake's ``find_package``
  function.

  .. code-block:: cmake

    _vtk_module_export_properties(
      [MODULE       <module>]
      [KIT          <kit>]
      BUILD_FILE    <path>
      INSTALL_FILE  <path>
      [PROPERTIES               <property>...]
      [FROM_GLOBAL_PROPERTIES   <property fragment>...]
      [SPLIT_INSTALL_PROPERTIES <property fragment>...])

  The ``BUILD_FILE`` and ``INSTALL_FILE`` arguments are required. Exactly one of
  ``MODULE`` and ``KIT`` is also required. The ``MODULE`` or ``KIT`` argument holds the
  name of the module or kit that will have properties exported. The ``BUILD_FILE``
  and ``INSTALL_FILE`` paths are *appended to*. As such, when setting up these
  files, it should be preceded with:

  .. code-block:: cmake

    file(WRITE "${build_file}")
    file(WRITE "${install_file}")

  To avoid accidental usage of the install file from the build tree, it is
  recommended to store it under a ``CMakeFiles/`` directory in the build tree with
  an additional ``.install`` suffix and use ``install(RENAME)`` to rename it at
  install time.

  The set of properties exported is computed as follows:

  * ``PROPERTIES`` queries the module target for the given property and exports
    its value as-is to both the build and install files. In addition, these
    properties are set on the target directly as the same name.
  * ``FROM_GLOBAL_PROPERTIES`` queries the global
    ``_vtk_module_<MODULE>_<fragment>`` property and exports it to both the build
    and install files as ``INTERFACE_vtk_module_<fragment>``.
  * ``SPLIT_INSTALL_PROPERTIES`` queries the target for
    ``INTERFACE_vtk_module_<fragment>`` and exports its value to the build file
    and ``INTERFACE_vtk_module_<fragment>_install`` to the install file as the
    non-install property name. This is generally useful for properties which
    change between the build and installation.
#]==]
function (_vtk_module_export_properties)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_export_properties
    ""
    "BUILD_FILE;INSTALL_FILE;MODULE;KIT"
    "FROM_GLOBAL_PROPERTIES;PROPERTIES;SPLIT_INSTALL_PROPERTIES")

  if (_vtk_export_properties_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for _vtk_export_properties: "
      "${_vtk_export_properties_UNPARSED_ARGUMENTS}.")
  endif ()

  if (DEFINED _vtk_export_properties_MODULE)
    if (DEFINED _vtk_export_properties_KIT)
      message(FATAL_ERROR
        "Only one of `MODULE` or `KIT` is required to export properties.")
    endif ()
    set(_vtk_export_properties_type "module")
    set(_vtk_export_properties_name "${_vtk_export_properties_MODULE}")
  elseif (_vtk_export_properties_KIT)
    set(_vtk_export_properties_type "kit")
    set(_vtk_export_properties_name "${_vtk_export_properties_KIT}")
  else ()
    message(FATAL_ERROR
      "A module or kit is required to export properties.")
  endif ()

  if (NOT _vtk_export_properties_BUILD_FILE)
    message(FATAL_ERROR
      "Exporting properties requires a build file to write to.")
  endif ()

  if (NOT _vtk_export_properties_INSTALL_FILE)
    message(FATAL_ERROR
      "Exporting properties requires an install file to write to.")
  endif ()

  if (_vtk_export_properties_type STREQUAL "module")
    _vtk_module_real_target(_vtk_export_properties_target_name "${_vtk_export_properties_name}")
  elseif (_vtk_export_properties_type STREQUAL "kit")
    _vtk_module_real_target_kit(_vtk_export_properties_target_name "${_vtk_export_properties_name}")
  endif ()

  foreach (_vtk_export_properties_global IN LISTS _vtk_export_properties_FROM_GLOBAL_PROPERTIES)
    get_property(_vtk_export_properties_is_set GLOBAL
      PROPERTY  "_vtk_${_vtk_export_properties_type}_${_vtk_export_properties_name}_${_vtk_export_properties_global}"
      SET)
    if (NOT _vtk_export_properties_is_set)
      continue ()
    endif ()

    get_property(_vtk_export_properties_value GLOBAL
      PROPERTY  "_vtk_${_vtk_export_properties_type}_${_vtk_export_properties_name}_${_vtk_export_properties_global}")
    set(_vtk_export_properties_set_property
      "set_property(TARGET \"${_vtk_export_properties_name}\" PROPERTY \"INTERFACE_vtk_${_vtk_export_properties_type}_${_vtk_export_properties_global}\" \"${_vtk_export_properties_value}\")\n")

    set_property(TARGET "${_vtk_export_properties_target_name}"
      PROPERTY
        "INTERFACE_vtk_${_vtk_export_properties_type}_${_vtk_export_properties_global}" "${_vtk_export_properties_value}")
    file(APPEND "${_vtk_export_properties_BUILD_FILE}"
      "${_vtk_export_properties_set_property}")
    file(APPEND "${_vtk_export_properties_INSTALL_FILE}"
      "${_vtk_export_properties_set_property}")
  endforeach ()

  foreach (_vtk_export_properties_target IN LISTS _vtk_export_properties_PROPERTIES)
    get_property(_vtk_export_properties_is_set
      TARGET    "${_vtk_export_properties_target_name}"
      PROPERTY  "${_vtk_export_properties_target}"
      SET)
    if (NOT _vtk_export_properties_is_set)
      continue ()
    endif ()

    get_property(_vtk_export_properties_value
      TARGET    "${_vtk_export_properties_target_name}"
      PROPERTY  "${_vtk_export_properties_target}")
    set(_vtk_export_properties_set_property
      "set_property(TARGET \"${_vtk_export_properties_name}\" PROPERTY \"${_vtk_export_properties_target}\" \"${_vtk_export_properties_value}\")\n")

    file(APPEND "${_vtk_export_properties_BUILD_FILE}"
      "${_vtk_export_properties_set_property}")
    file(APPEND "${_vtk_export_properties_INSTALL_FILE}"
      "${_vtk_export_properties_set_property}")
  endforeach ()

  foreach (_vtk_export_properties_split IN LISTS _vtk_export_properties_SPLIT_INSTALL_PROPERTIES)
    get_property(_vtk_export_properties_is_set
      TARGET    "${_vtk_export_properties_target_name}"
      PROPERTY  "INTERFACE_vtk_${_vtk_export_properties_type}_${_vtk_export_properties_split}"
      SET)
    if (NOT _vtk_export_properties_is_set)
      continue ()
    endif ()

    get_property(_vtk_export_properties_value
      TARGET    "${_vtk_export_properties_target_name}"
      PROPERTY  "INTERFACE_vtk_${_vtk_export_properties_type}_${_vtk_export_properties_split}")
    set(_vtk_export_properties_set_property
      "set_property(TARGET \"${_vtk_export_properties_name}\" PROPERTY \"INTERFACE_vtk_module_${_vtk_export_properties_split}\" \"${_vtk_export_properties_value}\")\n")
    file(APPEND "${_vtk_export_properties_BUILD_FILE}"
      "${_vtk_export_properties_set_property}")

    get_property(_vtk_export_properties_value
      TARGET    "${_vtk_export_properties_target_name}"
      PROPERTY  "INTERFACE_vtk_${_vtk_export_properties_type}_${_vtk_export_properties_split}_install")
    set(_vtk_export_properties_set_property
      "set_property(TARGET \"${_vtk_export_properties_name}\" PROPERTY \"INTERFACE_vtk_module_${_vtk_export_properties_split}\" \"${_vtk_export_properties_value}\")\n")
    file(APPEND "${_vtk_export_properties_INSTALL_FILE}"
      "${_vtk_export_properties_set_property}")
  endforeach ()
endfunction ()

include("${CMAKE_CURRENT_LIST_DIR}/vtkModuleTesting.cmake")

#[==[.rst:
.. cmake:command:: vtk_module_build

  Build modules and kits. |module|

  Once all of the modules have been scanned, they need to be built. Generally,
  there will be just one build necessary for a set of scans, though they may be
  built distinctly as well. If there are multiple calls to this function, they
  should generally in reverse order of their scans.

  .. code-block:: cmake

    vtk_module_build(
      MODULES       <module>...
      [KITS          <kit>...]

      [LIBRARY_NAME_SUFFIX  <suffix>]
      [VERSION              <version>]
      [SOVERSION            <soversion>]

      [PACKAGE              <package>]

      [BUILD_WITH_KITS  <ON|OFF>]

      [ENABLE_WRAPPING <ON|OFF>]
      [ENABLE_SERIALIZATION <ON|OFF>]

      [USE_EXTERNAL <ON|OFF>]

      [INSTALL_HEADERS             <ON|OFF>]
      [HEADERS_COMPONENT           <component>]
      [HEADERS_EXCLUDE_FROM_ALL    <ON|OFF>]
      [USE_FILE_SETS               <ON|OFF>]

      [TARGETS_COMPONENT  <component>]
      [INSTALL_EXPORT     <export>]

      [TARGET_SPECIFIC_COMPONENTS <ON|OFF>]

      [LICENSE_COMPONENT  <component>]

      [UTILITY_TARGET     <target>]

      [TEST_DIRECTORY_NAME        <name>]
      [TEST_DATA_TARGET           <target>]
      [TEST_INPUT_DATA_DIRECTORY  <directory>]
      [TEST_OUTPUT_DATA_DIRECTORY <directory>]
      [TEST_OUTPUT_DIRECTORY      <directory>]

      [GENERATE_SPDX            <ON|OFF>]
      [SPDX_COMPONENT           <component>]
      [SPDX_DOCUMENT_NAMESPACE  <uri>]
      [SPDX_DOWNLOAD_LOCATION   <url>]

      [ARCHIVE_DESTINATION    <destination>]
      [HEADERS_DESTINATION    <destination>]
      [LIBRARY_DESTINATION    <destination>]
      [RUNTIME_DESTINATION    <destination>]
      [CMAKE_DESTINATION      <destination>]
      [LICENSE_DESTINATION    <destination>]
      [HIERARCHY_DESTINATION  <destination>])

  The only requirement of the function is the list of modules to build, the rest
  have reasonable defaults if not specified.

  * ``MODULES``: (Required) The list of modules to build.
  * ``KITS``: (Required if ``BUILD_WITH_KITS`` is ``ON``) The list of kits to build.
  * ``LIBRARY_NAME_SUFFIX``: (Defaults to ``""``) A suffix to add to library names.
    If it is not empty, it is prefixed with ``-`` to separate it from the kit
    name.
  * ``VERSION``: If specified, the ``VERSION`` property on built libraries will be
    set to this value.
  * ``SOVERSION``: If specified, the ``SOVERSION`` property on built libraries will
    be set to this value.
  * ``PACKAGE``: (Defaults to ``${CMAKE_PROJECT_NAME}``) The name the build is
    meant to be found as when using ``find_package``. Note that separate builds
    will require distinct ``PACKAGE`` values.
  * ``BUILD_WITH_KITS``: (Defaults to ``OFF``) If enabled, kit libraries will be
    built.
  * ``ENABLE_WRAPPING``: (Default depends on the existence of
    ``VTK::WrapHierarchy`` or ``VTKCompileTools::WrapHierarchy`` targets) If
    enabled, wrapping will be available to the modules built in this call.
  * ``ENABLE_SERIALIZATION``: (Defaults to ``OFF``) If enabled, (de)serialization
    code will be autogenerated for classes with the correct wrapping hints.
  * ``USE_EXTERNAL``: (Defaults to ``OFF``) Whether third party modules should find
    external copies rather than building their own copy.
  * ``INSTALL_HEADERS``: (Defaults to ``ON``) Whether or not to install public headers.
  * ``HEADERS_COMPONENT``: (Defaults to ``development``) The install component to
    use for header installation. Note that other SDK-related bits use the same
    component (e.g., CMake module files).
  * ``HEADERS_EXCLUDE_FROM_ALL``: (Defaults to ``OFF``) Whether to install the headers
    component in ALL or not.
  * ``USE_FILE_SETS``: (Defaults to ``OFF``) Whether to use ``FILE_SET`` source
    specification or not.
  * ``TARGETS_COMPONENT``: ``Defaults to ``runtime``) The install component to use
    for the libraries built.
  * ``TARGET_SPECIFIC_COMPONENTS``: (Defaults to ``OFF``) If ``ON``, place artifacts
    into target-specific install components (``<TARGET>-<COMPONENT>``).
  * ``LICENSE_COMPONENT``: (Defaults to ``licenses``) The install component to use
    for licenses.
  * ``UTILITY_TARGET``: If specified, all libraries and executables made by the
    VTK Module API will privately link to this target. This may be used to
    provide things such as project-wide compilation flags or similar.
  * ``TARGET_NAMESPACE``: ``Defaults to ``\<AUTO\>``) The namespace for installed
    targets. All targets must have the same namespace. If set to ``\<AUTO\>``,
    the namespace will be detected automatically.
  * ``INSTALL_EXPORT``: (Defaults to ``""``) If non-empty, targets will be added to
    the given export. The export will also be installed as part of this build
    command.
  * ``TEST_DIRECTORY_NAME``: (Defaults to ``Testing``) The name of the testing
    directory to look for in each module. Set to ``NONE`` to disable automatic
    test management.
  * ``TEST_DATA_TARGET``: (Defaults to ``<PACKAGE>-data``) The target to add
    testing data download commands to.
  * ``TEST_INPUT_DATA_DIRECTORY``: (Defaults to
    ``${CMAKE_CURRENT_SOURCE_DIR}/Data``) The directory which will contain data
    for use by tests.
  * ``TEST_OUTPUT_DATA_DIRECTORY``: (Defaults to
    ``${CMAKE_CURRENT_BINARY_DIR}/Data``) The directory which will contain data
    for use by tests.
  * ``TEST_OUTPUT_DIRECTORY``: (Defaults to
    ``${CMAKE_BINARY_DIR}/<TEST_DIRECTORY_NAME>/Temporary``) The directory which
    tests may write any output files to.
  * ``GENERATE_SPDX``: (Defaults to ``OFF``) Whether or not to generate and install
    SPDX file for each modules and third parties.
  * ``SPDX_COMPONENT``: (Defaults to ``spdx``) The install component to use
    for SPDX files.
  * ``SPDX_DOCUMENT_NAMESPACE``: (Defaults to ``""``) Document namespace to use when
    generating SPDX files.
  * ``SPDX_DOWNLOAD_LOCATION``: (Defaults to ``""``) Download location to use when
    generating SPDX files.

  The remaining arguments control where to install files related to the build.
  See CMake documentation for the difference between ``ARCHIVE``, ``LIBRARY``, and
  ``RUNTIME``.

  * ``ARCHIVE_DESTINATION``: (Defaults to ``${CMAKE_INSTALL_LIBDIR}``) The install
    destination for archive files.
  * ``HEADERS_DESTINATION``: (Defaults to ``${CMAKE_INSTALL_INCLUDEDIR}``) The
    install destination for header files.
  * ``LIBRARY_DESTINATION``: (Defaults to ``${CMAKE_INSTALL_LIBDIR}``) The install
    destination for library files.
  * ``RUNTIME_DESTINATION``: (Defaults to ``${CMAKE_INSTALL_BINDIR}``) The install
    destination for runtime files.
  * ``CMAKE_DESTINATION``: (Defaults to ``<LIBRARY_DESTINATION>/cmake/<PACKAGE>``)
    The install destination for CMake files.
  * ``LICENSE_DESTINATION``: (Defaults to ``${CMAKE_INSTALL_DATAROOTDIR}/licenses/${CMAKE_PROJECT_NAME}``)
    The install destination for license files.
  * ``SPDX_DESTINATION``: (Defaults to ``${CMAKE_INSTALL_DATAROOTDIR}/doc/${CMAKE_PROJECT_NAME}/spdx/``)
    The install destination for SPDX files.
  * ``HIERARCHY_DESTINATION``: (Defaults to
    ``<LIBRARY_DESTINATION>/vtk/hierarchy/<PACKAGE>``) The install destination
    for hierarchy files (used for language wrapping).
#]==]
function (vtk_module_build)
  set(_vtk_build_install_arguments
    # Headers
    INSTALL_HEADERS
    HEADERS_COMPONENT
    HEADERS_EXCLUDE_FROM_ALL
    USE_FILE_SETS

    # Targets
    INSTALL_EXPORT
    TARGETS_COMPONENT
    LICENSE_COMPONENT
    SPDX_COMPONENT
    TARGET_NAMESPACE
    UTILITY_TARGET

    # Destinations
    ARCHIVE_DESTINATION
    HEADERS_DESTINATION
    LIBRARY_DESTINATION
    RUNTIME_DESTINATION
    CMAKE_DESTINATION
    LICENSE_DESTINATION
    SPDX_DESTINATION
    HIERARCHY_DESTINATION)
  set(_vtk_build_test_arguments
    # Testing
    TEST_DIRECTORY_NAME
    TEST_DATA_TARGET
    TEST_INPUT_DATA_DIRECTORY
    TEST_OUTPUT_DATA_DIRECTORY
    TEST_OUTPUT_DIRECTORY)
  set(_vtk_spdx_arguments
    # SPDX related arguments
    GENERATE_SPDX
    SPDX_DOCUMENT_NAMESPACE
    SPDX_DOWNLOAD_LOCATION)

  # TODO: Add an option to build statically? Currently, `BUILD_SHARED_LIBS` is
  # used.

  cmake_parse_arguments(PARSE_ARGV 0 _vtk_build
    ""
    "BUILD_WITH_KITS;USE_EXTERNAL;TARGET_SPECIFIC_COMPONENTS;LIBRARY_NAME_SUFFIX;VERSION;SOVERSION;PACKAGE;ENABLE_WRAPPING;ENABLE_SERIALIZATION;${_vtk_build_install_arguments};${_vtk_build_test_arguments};${_vtk_spdx_arguments}"
    "MODULES;KITS")

  if (_vtk_build_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_build: "
      "${_vtk_build_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_build_USE_EXTERNAL)
    set(_vtk_build_USE_EXTERNAL OFF)
  endif ()

  if (NOT DEFINED _vtk_build_TARGET_SPECIFIC_COMPONENTS)
    set(_vtk_build_TARGET_SPECIFIC_COMPONENTS OFF)
  endif ()

  if (NOT DEFINED _vtk_build_PACKAGE)
    set(_vtk_build_PACKAGE "${CMAKE_PROJECT_NAME}")
  endif ()
  get_property(_vtk_build_package_exists GLOBAL
    PROPERTY  "_vtk_module_package_${_vtk_build_PACKAGE}"
    SET)
  if (_vtk_build_package_exists)
    message(FATAL_ERROR
      "A set of modules have already been built using the "
      "`${_vtk_build_PACKAGE}` package.")
  else ()
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_package_${_vtk_build_PACKAGE}" "ON")
  endif ()

  if (NOT DEFINED _vtk_build_INSTALL_HEADERS)
    set(_vtk_build_INSTALL_HEADERS ON)
  endif ()

  if (NOT DEFINED _vtk_build_HEADERS_EXCLUDE_FROM_ALL)
    set(_vtk_build_HEADERS_EXCLUDE_FROM_ALL OFF)
  endif ()

  if (NOT DEFINED _vtk_build_USE_FILE_SETS)
    set(_vtk_build_USE_FILE_SETS OFF)
  endif ()

  if (NOT DEFINED _vtk_build_ENABLE_WRAPPING)
    if (TARGET "VTKCompileTools::WrapHierarchy" OR
        TARGET "VTK::WrapHierarchy")
      set(_vtk_build_ENABLE_WRAPPING ON)
    else ()
      set(_vtk_build_ENABLE_WRAPPING OFF)
    endif ()
  endif ()

  if (NOT DEFINED _vtk_build_ENABLE_SERIALIZATION)
    set(_vtk_build_ENABLE_SERIALIZATION OFF)
  endif ()

  if (_vtk_build_ENABLE_SERIALIZATION AND NOT _vtk_build_ENABLE_WRAPPING)
    message(FATAL_ERROR
      "ENABLE_SERIALIZATION requires that ENABLE_WRAPPING is turned ON.")
  endif ()

  if (NOT DEFINED _vtk_build_TARGET_NAMESPACE)
    set(_vtk_build_TARGET_NAMESPACE "<AUTO>")
  endif ()

  if (NOT DEFINED _vtk_build_BUILD_WITH_KITS)
    set(_vtk_build_BUILD_WITH_KITS OFF)
  endif ()
  if (_vtk_build_BUILD_WITH_KITS AND NOT BUILD_SHARED_LIBS)
    message(AUTHOR_WARNING
      "Static builds with kits are not well-tested and doesn't make much "
      "sense. It is recommended to only build with kits in shared builds.")
  endif ()

  if (_vtk_build_BUILD_WITH_KITS AND NOT DEFINED _vtk_build_KITS)
    message(FATAL_ERROR
      "Building with kits was requested, but no kits were specified.")
  endif ()

  if (NOT DEFINED _vtk_build_TEST_DIRECTORY_NAME)
    set(_vtk_build_TEST_DIRECTORY_NAME "Testing")
  endif ()

  if (NOT DEFINED _vtk_build_TEST_DATA_TARGET)
    set(_vtk_build_TEST_DATA_TARGET "${_vtk_build_PACKAGE}-data")
  endif ()

  if (NOT DEFINED _vtk_build_TEST_INPUT_DATA_DIRECTORY)
    set(_vtk_build_TEST_INPUT_DATA_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Data")
  endif ()

  if (NOT DEFINED _vtk_build_TEST_OUTPUT_DATA_DIRECTORY)
    set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Data")
  endif ()

  if (NOT DEFINED _vtk_build_TEST_OUTPUT_DIRECTORY)
    set(_vtk_build_TEST_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_vtk_build_TEST_DIRECTORY_NAME}/Temporary")
  endif ()

  if (NOT DEFINED _vtk_build_HEADERS_COMPONENT)
    set(_vtk_build_HEADERS_COMPONENT "development")
  endif ()

  if (NOT DEFINED _vtk_build_LICENSE_COMPONENT)
    set(_vtk_build_LICENSE_COMPONENT "licenses")
  endif ()

  if (NOT DEFINED _vtk_build_SPDX_COMPONENT)
    set(_vtk_build_SPDX_COMPONENT "spdx")
  endif ()

  if (NOT DEFINED _vtk_build_GENERATE_SPDX)
    set(_vtk_build_GENERATE_SPDX OFF)
  endif ()

  if (NOT DEFINED _vtk_build_ARCHIVE_DESTINATION)
    set(_vtk_build_ARCHIVE_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  endif ()

  if (NOT DEFINED _vtk_build_HEADERS_DESTINATION)
    set(_vtk_build_HEADERS_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  endif ()

  if (NOT DEFINED _vtk_build_LIBRARY_DESTINATION)
    set(_vtk_build_LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
  endif ()

  if (NOT DEFINED _vtk_build_RUNTIME_DESTINATION)
    set(_vtk_build_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
  endif ()

  if (NOT DEFINED _vtk_build_CMAKE_DESTINATION)
    set(_vtk_build_CMAKE_DESTINATION "${_vtk_build_LIBRARY_DESTINATION}/cmake/${_vtk_build_PACKAGE}")
  endif ()

  if (NOT DEFINED _vtk_build_LICENSE_DESTINATION)
    set(_vtk_build_LICENSE_DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${CMAKE_PROJECT_NAME}")
  endif ()

  if (NOT DEFINED _vtk_build_SPDX_DESTINATION)
    set(_vtk_build_SPDX_DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/doc/${CMAKE_PROJECT_NAME}/spdx/")
  endif ()

  if (NOT DEFINED _vtk_build_HIERARCHY_DESTINATION)
    set(_vtk_build_HIERARCHY_DESTINATION "${_vtk_build_LIBRARY_DESTINATION}/vtk/hierarchy/${_vtk_build_PACKAGE}")
  endif ()

  if (NOT DEFINED _vtk_build_TARGETS_COMPONENT)
    set(_vtk_build_TARGETS_COMPONENT "runtime")
  endif ()

  if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_vtk_build_ARCHIVE_DESTINATION}")
  endif ()
  if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_vtk_build_LIBRARY_DESTINATION}")
  endif ()
  if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_vtk_build_RUNTIME_DESTINATION}")
  endif ()

  if (NOT _vtk_build_MODULES)
    message(FATAL_ERROR
      "No modules given to build.")
  endif ()

  _vtk_module_check_destinations(_vtk_build_
    ARCHIVE_DESTINATION
    HEADERS_DESTINATION
    RUNTIME_DESTINATION
    CMAKE_DESTINATION
    LICENSE_DESTINATION
    SPDX_DESTINATION
    HIERARCHY_DESTINATION)

  foreach (_vtk_build_module IN LISTS _vtk_build_MODULES)
    get_property("_vtk_build_${_vtk_build_module}_depends" GLOBAL
      PROPERTY "_vtk_module_${_vtk_build_module}_depends")
    get_property("_vtk_build_${_vtk_build_module}_private_depends" GLOBAL
      PROPERTY "_vtk_module_${_vtk_build_module}_private_depends")
    get_property("_vtk_build_${_vtk_build_module}_optional_depends" GLOBAL
      PROPERTY "_vtk_module_${_vtk_build_module}_optional_depends")
    get_property("_vtk_build_${_vtk_build_module}_order_depends" GLOBAL
      PROPERTY "_vtk_module_${_vtk_build_module}_order_depends")
    set("_vtk_build_${_vtk_build_module}_all_depends"
      ${_vtk_build_${_vtk_build_module}_depends}
      ${_vtk_build_${_vtk_build_module}_private_depends}
      ${_vtk_build_${_vtk_build_module}_optional_depends}
      ${_vtk_build_${_vtk_build_module}_order_depends})
  endforeach ()

  set(_vtk_build_sorted_modules "${_vtk_build_MODULES}")
  vtk_topological_sort(_vtk_build_sorted_modules "_vtk_build_" "_all_depends")

  foreach (_vtk_build_module IN LISTS _vtk_build_sorted_modules)
    if (NOT _vtk_build_module IN_LIST _vtk_build_MODULES)
      continue ()
    endif ()

    if (TARGET "${_vtk_build_module}")
      get_property(_vtk_build_is_imported
        TARGET    "${_vtk_build_module}"
        PROPERTY  IMPORTED)

      # TODO: Is this right?
      if (NOT _vtk_build_is_imported)
        message(FATAL_ERROR
          "The ${_vtk_build_module} module has been requested to be built, but "
          "it is already built by this project.")
      endif ()

      continue ()
    endif ()

    foreach (_vtk_build_depend IN LISTS "_vtk_build_${_vtk_build_module}_depends" "_vtk_build_${_vtk_build_module}_private_depends")
      if (NOT TARGET "${_vtk_build_depend}")
        get_property(_vtk_build_enable_tests_by_want GLOBAL
          PROPERTY  "_vtk_module_${_vtk_build_module}_enable_tests_by_want")

        set(_vtk_build_explain "")
        if (_vtk_build_enable_tests_by_want)
          string(APPEND _vtk_build_explain
            " The `vtk_module_scan` for this module used `ENABLE_TESTS WANT`. "
            "This is a known issue, but the fix is not trivial. You may "
            "either change the flag used to control testing for this scan or "
            "explicitly enable the ${_vtk_build_depend} module.")
        endif ()

        message(FATAL_ERROR
          "The ${_vtk_build_depend} dependency is missing for "
          "${_vtk_build_module}.${_vtk_build_explain}")
      endif ()
    endforeach ()

    get_property(_vtk_build_module_file GLOBAL
      PROPERTY  "_vtk_module_${_vtk_build_module}_file")
    if (NOT _vtk_build_module_file)
      message(FATAL_ERROR
        "The requested ${_vtk_build_module} module is not a VTK module.")
    endif ()

    _vtk_module_debug(building "@_vtk_build_module@ is being built")

    get_filename_component(_vtk_build_module_dir "${_vtk_build_module_file}" DIRECTORY)
    if (COMMAND cmake_path) # XXX(cmake-3.20)
      cmake_path(NORMAL_PATH _vtk_build_module_dir)
    else ()
      get_filename_component(_vtk_build_module_dir "${_vtk_build_module_dir}" ABSOLUTE)
    endif ()
    file(RELATIVE_PATH _vtk_build_module_subdir "${CMAKE_SOURCE_DIR}" "${_vtk_build_module_dir}")
    set(_vtk_build_module_subdir_build "${_vtk_build_module_subdir}")

    # Check if the source for this module is outside of `CMAKE_SOURCE_DIR`.
    # Place it under `CMAKE_BINARY_DIR` more meaningfully if so.
    if (_vtk_build_module_subdir MATCHES "\\.\\./")
      file(RELATIVE_PATH _vtk_build_module_subdir_build "${CMAKE_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
      get_property(_vtk_build_module_library_name GLOBAL
        PROPERTY "_vtk_module_${_vtk_build_module}_library_name")
      string(APPEND _vtk_build_module_subdir_build "/${_vtk_build_module_library_name}")
    endif ()

    add_subdirectory(
      "${CMAKE_SOURCE_DIR}/${_vtk_build_module_subdir}"
      "${CMAKE_BINARY_DIR}/${_vtk_build_module_subdir_build}")

    if (NOT TARGET "${_vtk_build_module}")
      message(FATAL_ERROR
        "The ${_vtk_build_module} is being built, but a matching target was "
        "not created.")
    endif ()
  endforeach ()

  if (_vtk_build_BUILD_WITH_KITS)
    foreach (_vtk_build_kit IN LISTS _vtk_build_KITS)
      get_property(_vtk_build_target_name GLOBAL
        PROPERTY  "_vtk_kit_${_vtk_build_kit}_target_name")
      set(_vtk_kit_source_file
        "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/vtk_module_kit_${_vtk_build_target_name}.c")
      file(GENERATE
        OUTPUT  "${_vtk_kit_source_file}"
        CONTENT "void vtk_module_kit_${_vtk_build_target_name}(void);\nvoid vtk_module_kit_${_vtk_build_target_name}(void) {}\n")
      add_library("${_vtk_build_target_name}"
        "${_vtk_kit_source_file}")
      get_property(_vtk_build_namespace GLOBAL
        PROPERTY  "_vtk_kit_${_vtk_build_kit}_namespace")
      if (_vtk_build_TARGET_NAMESPACE STREQUAL "<AUTO>")
        set(_vtk_build_TARGET_NAMESPACE "${_vtk_build_namespace}")
      endif ()
      if (NOT _vtk_build_namespace STREQUAL _vtk_build_TARGET_NAMESPACE)
        message(FATAL_ERROR
          "The `TARGET_NAMESPACE` (${_vtk_build_TARGET_NAMESPACE}) is not the "
          "same as the ${_vtk_build_kit} kit namespace "
          "(${_vtk_build_namespace}).")
      endif ()
      if (NOT _vtk_build_kit STREQUAL _vtk_build_target_name)
        add_library("${_vtk_build_kit}" ALIAS
          "${_vtk_build_target_name}")
      endif ()
      _vtk_module_apply_properties("${_vtk_build_target_name}")
      _vtk_module_install("${_vtk_build_target_name}")

      set(_vtk_build_kit_modules_object_libraries)
      set(_vtk_build_kit_modules_private_depends)

      get_property(_vtk_build_kit_modules GLOBAL
        PROPERTY  "_vtk_kit_${_vtk_build_kit}_kit_modules")
      foreach (_vtk_build_kit_module IN LISTS _vtk_build_kit_modules)
        get_property(_vtk_build_kit_module_target_name GLOBAL
          PROPERTY "_vtk_module_${_vtk_build_kit_module}_target_name")
        list(APPEND _vtk_build_kit_modules_object_libraries
          "${_vtk_build_kit_module_target_name}-objects")

        _vtk_private_kit_link_target("${_vtk_build_kit_module}"
          USAGE_TARGET_NAME _vtk_build_kit_module_usage_name)
        if (TARGET "${_vtk_build_kit_module_usage_name}")
          list(APPEND _vtk_build_kit_modules_private_depends
            "${_vtk_build_kit_module_usage_name}")
        endif ()

        # Since there is no link step for modules, we need to copy the private
        # dependencies of the constituent modules into the kit so that their
        # private dependencies are actually linked.
        get_property(_vtk_build_kit_module_private_depends GLOBAL
          PROPERTY "_vtk_module_${_vtk_build_kit_module}_private_depends")
        # Also grab optional dependencies since they end up being private
        # links.
        get_property(_vtk_build_kit_module_optional_depends GLOBAL
          PROPERTY "_vtk_module_${_vtk_build_kit_module}_optional_depends")
        foreach (_vtk_build_kit_module_private_depend IN LISTS _vtk_build_kit_module_private_depends _vtk_build_kit_module_optional_depends)
          if (_vtk_build_kit_module_private_depend IN_LIST _vtk_build_kit_module_optional_depends)
            _vtk_module_optional_dependency_exists("${_vtk_build_kit_module_private_depend}"
              SATISFIED_VAR _vtk_build_kit_module_has_optional_dep)
            if (NOT _vtk_build_kit_module_has_optional_dep)
              continue ()
            endif ()
          endif ()

          # But we don't need to link to modules that are part of the kit we are
          # building.
          if (NOT _vtk_build_kit_module_private_depend IN_LIST _vtk_build_kit_modules)
            list(APPEND _vtk_build_kit_modules_private_depends
              "$<LINK_ONLY:${_vtk_build_kit_module_private_depend}>")
          endif ()
        endforeach ()
      endforeach ()

      if (_vtk_build_kit_modules_private_depends)
        list(REMOVE_DUPLICATES _vtk_build_kit_modules_private_depends)
      endif ()
      if (_vtk_build_kit_modules_private_links)
        list(REMOVE_DUPLICATES _vtk_build_kit_modules_private_links)
      endif ()

      target_link_libraries("${_vtk_build_target_name}"
        PRIVATE
          ${_vtk_build_kit_modules_object_libraries}
          ${_vtk_build_kit_modules_private_depends})

      if (_vtk_build_UTILITY_TARGET)
        target_link_libraries("${_vtk_build_target_name}"
          PRIVATE
            "${_vtk_build_UTILITY_TARGET}")
      endif ()

      get_property(_vtk_build_kit_library_name GLOBAL
        PROPERTY "_vtk_kit_${_vtk_build_kit}_library_name")
      if (_vtk_build_LIBRARY_NAME_SUFFIX)
        string(APPEND _vtk_build_kit_library_name "-${_vtk_build_LIBRARY_NAME_SUFFIX}")
      endif ()
      set_target_properties("${_vtk_build_target_name}"
        PROPERTIES
          OUTPUT_NAME "${_vtk_build_kit_library_name}")
    endforeach ()
  endif ()

  set(_vtk_build_properties_filename "${_vtk_build_PACKAGE}-vtk-module-properties.cmake")
  set(_vtk_build_properties_install_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_build_properties_filename}.install")
  set(_vtk_build_properties_build_file "${CMAKE_BINARY_DIR}/${_vtk_build_CMAKE_DESTINATION}/${_vtk_build_properties_filename}")

  file(WRITE "${_vtk_build_properties_build_file}")

  _vtk_module_write_import_prefix(
    "${_vtk_build_properties_install_file}"
    "${_vtk_build_CMAKE_DESTINATION}")

  foreach (_vtk_build_module IN LISTS _vtk_build_MODULES)
    get_property(_vtk_build_namespace GLOBAL
      PROPERTY  "_vtk_module_${_vtk_build_module}_namespace")
    if (_vtk_build_TARGET_NAMESPACE STREQUAL "<AUTO>")
      set(_vtk_build_TARGET_NAMESPACE "${_vtk_build_namespace}")
    endif ()
    if (NOT _vtk_build_namespace STREQUAL _vtk_build_TARGET_NAMESPACE)
      message(FATAL_ERROR
        "The `TARGET_NAMESPACE` (${_vtk_build_TARGET_NAMESPACE}) is not the "
        "same as the ${_vtk_build_module} module namespace "
        "(${_vtk_build_namespace}).")
    endif ()

    get_property(_vtk_build_is_third_party
      TARGET    "${_vtk_build_module}"
      PROPERTY  "INTERFACE_vtk_module_third_party")
    if (_vtk_build_is_third_party)
      _vtk_module_export_properties(
        BUILD_FILE    "${_vtk_build_properties_build_file}"
        INSTALL_FILE  "${_vtk_build_properties_install_file}"
        MODULE        "${_vtk_build_module}"
        FROM_GLOBAL_PROPERTIES
          # Export the dependencies of a module.
          depends
          private_depends
          optional_depends
          # The library name of the module.
          library_name
        PROPERTIES
          # Export whether a module is third party or not.
          INTERFACE_vtk_module_third_party
          INTERFACE_vtk_module_exclude_wrap
          INTERFACE_vtk_module_include_marshal)
      continue ()
    endif ()

    set(_vtk_build_split_properties)
    get_property(_vtk_build_exclude_wrap
      TARGET    "${_vtk_build_module}"
      PROPERTY  "INTERFACE_vtk_module_${_vtk_build_module}_exclude_wrap")
    if (NOT _vtk_build_exclude_wrap)
      list(APPEND _vtk_build_split_properties
        headers)
      if (_vtk_build_ENABLE_WRAPPING)
        list(APPEND _vtk_build_split_properties
          hierarchy)
      endif ()
    endif ()

    set(_vtk_build_properties_kit_properties)
    if (_vtk_build_BUILD_WITH_KITS)
      list(APPEND _vtk_build_properties_kit_properties
        # Export the kit membership of a module.
        kit)
    endif ()

    _vtk_module_export_properties(
      BUILD_FILE    "${_vtk_build_properties_build_file}"
      INSTALL_FILE  "${_vtk_build_properties_install_file}"
      MODULE        "${_vtk_build_module}"
      FROM_GLOBAL_PROPERTIES
        # Export whether the module should be excluded from wrapping or not.
        exclude_wrap
        # Export whether the module opts into automatic marshalling or not.
        include_marshal
        # Export the dependencies of a module.
        depends
        private_depends
        optional_depends
        # Export what modules are implemented by the module.
        implements
        # Export whether the module contains autoinit logic.
        implementable
        # The library name of the module.
        library_name
        ${_vtk_build_properties_kit_properties}
      PROPERTIES
        # Export whether the module needs autoinit logic handled.
        INTERFACE_vtk_module_needs_autoinit
        # Forward private usage requirements with global effects.
        INTERFACE_vtk_module_forward_link
      SPLIT_INSTALL_PROPERTIES
        # Set the properties which differ between build and install trees.
        ${_vtk_build_split_properties})
  endforeach ()

  if (_vtk_build_BUILD_WITH_KITS)
    foreach (_vtk_build_kit IN LISTS _vtk_build_KITS)
      _vtk_module_export_properties(
        BUILD_FILE    "${_vtk_build_properties_build_file}"
        INSTALL_FILE  "${_vtk_build_properties_install_file}"
        KIT           "${_vtk_build_kit}"
        FROM_GLOBAL_PROPERTIES
          # Export the list of modules in the kit.
          kit_modules)
    endforeach ()
  endif ()

  if (_vtk_build_INSTALL_EXPORT AND _vtk_build_INSTALL_HEADERS)
    set(_vtk_build_namespace)
    if (_vtk_build_TARGET_NAMESPACE)
      set(_vtk_build_namespace
        NAMESPACE "${_vtk_build_TARGET_NAMESPACE}::")
    endif ()

    export(
      EXPORT    "${_vtk_build_INSTALL_EXPORT}"
      ${_vtk_build_namespace}
      FILE      "${CMAKE_BINARY_DIR}/${_vtk_build_CMAKE_DESTINATION}/${_vtk_build_PACKAGE}-targets.cmake")

    set(_vtk_build_exclude_from_all "")
    if (_vtk_build_HEADERS_EXCLUDE_FROM_ALL)
      set(_vtk_build_exclude_from_all "EXCLUDE_FROM_ALL")
    endif ()

    install(
      EXPORT      "${_vtk_build_INSTALL_EXPORT}"
      DESTINATION "${_vtk_build_CMAKE_DESTINATION}"
      ${_vtk_build_namespace}
      FILE        "${_vtk_build_PACKAGE}-targets.cmake"
      COMPONENT   "${_vtk_build_HEADERS_COMPONENT}"
      ${_vtk_build_exclude_from_all})

    if (_vtk_build_INSTALL_HEADERS)
      file(APPEND "${_vtk_build_properties_install_file}"
        "unset(_vtk_module_import_prefix)\n")

      install(
        FILES       "${_vtk_build_properties_install_file}"
        DESTINATION "${_vtk_build_CMAKE_DESTINATION}"
        RENAME      "${_vtk_build_properties_filename}"
        COMPONENT   "${_vtk_build_HEADERS_COMPONENT}"
        ${_vtk_build_exclude_from_all})
    endif ()
  endif ()

  get_property(_vtk_build_test_modules GLOBAL
    PROPERTY "_vtk_module_test_modules")
  set(_vtk_build_tests_handled)
  foreach (_vtk_build_test IN LISTS _vtk_build_test_modules)
    if (NOT _vtk_build_test IN_LIST _vtk_build_MODULES)
      continue ()
    endif ()
    list(APPEND _vtk_build_tests_handled
      "${_vtk_build_test}")

    get_property(_vtk_build_test_depends GLOBAL
      PROPERTY "_vtk_module_${_vtk_build_test}_test_depends")

    set(_vtk_build_test_has_depends TRUE)
    foreach (_vtk_build_test_depend IN LISTS _vtk_build_test_depends)
      if (NOT TARGET "${_vtk_build_test_depend}")
        set(_vtk_build_test_has_depends FALSE)
        _vtk_module_debug(testing "@_vtk_build_test@ testing disabled due to missing @_vtk_build_test_depend@")
      endif ()
    endforeach ()
    if (NOT _vtk_build_test_has_depends)
      continue ()
    endif ()

    _vtk_module_debug(testing "@_vtk_build_test@ testing enabled")

    get_property(_vtk_build_module_file GLOBAL
      PROPERTY  "_vtk_module_${_vtk_build_test}_file")

    if (NOT _vtk_build_TEST_DIRECTORY_NAME STREQUAL "NONE")
      get_filename_component(_vtk_build_module_dir "${_vtk_build_module_file}" DIRECTORY)
      if (COMMAND cmake_path) # XXX(cmake-3.20)
        cmake_path(NORMAL_PATH _vtk_build_module_dir)
      else ()
        get_filename_component(_vtk_build_module_dir "${_vtk_build_module_dir}" ABSOLUTE)
      endif ()
      file(RELATIVE_PATH _vtk_build_module_subdir "${CMAKE_SOURCE_DIR}" "${_vtk_build_module_dir}")
      set(_vtk_build_module_subdir_build "${_vtk_build_module_subdir}")
      if (EXISTS "${CMAKE_SOURCE_DIR}/${_vtk_build_module_subdir}/${_vtk_build_TEST_DIRECTORY_NAME}")
        # Check if the source for this module is outside of `CMAKE_SOURCE_DIR`.
        # Place it under `CMAKE_BINARY_DIR` more meaningfully if so.
        if (_vtk_build_module_subdir MATCHES "\\.\\./")
          file(RELATIVE_PATH _vtk_build_module_subdir_build "${CMAKE_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
          get_property(_vtk_build_module_library_name GLOBAL
            PROPERTY "_vtk_module_${_vtk_build_test}_library_name")
          string(APPEND _vtk_build_module_subdir_build "/${_vtk_build_module_library_name}")
        endif ()

        get_property(_vtk_build_test_labels GLOBAL
          PROPERTY  "_vtk_module_${_vtk_build_test}_test_labels")
        add_subdirectory(
          "${CMAKE_SOURCE_DIR}/${_vtk_build_module_subdir}/${_vtk_build_TEST_DIRECTORY_NAME}"
          "${CMAKE_BINARY_DIR}/${_vtk_build_module_subdir_build}/${_vtk_build_TEST_DIRECTORY_NAME}")
      endif ()
    endif ()
  endforeach ()

  if (_vtk_build_test_modules AND _vtk_build_tests_handled)
    list(REMOVE_ITEM _vtk_build_test_modules
      ${_vtk_build_tests_handled})
    set_property(GLOBAL
      PROPERTY
        _vtk_module_test_modules "${_vtk_build_test_modules}")
  endif ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_standard_includes

  Add "standard" include directories to a module. |module-impl|

  Add the "standard" includes for a module to its interface. These are the source
  And build directories for the module itself. They are always either ``PUBLIC`` or
  ``INTERFACE`` (depending on the module's target type).

  .. code-block:: cmake

    _vtk_module_standard_includes(
      [SYSTEM]
      [INTERFACE]
      TARGET                <target>
      [HEADERS_DESTINATION  <destination>])
#]==]
function (_vtk_module_standard_includes)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_standard_includes
    "SYSTEM;INTERFACE"
    "TARGET;HEADERS_DESTINATION"
    "")

  if (NOT _vtk_standard_includes_TARGET)
    message(FATAL_ERROR
      "The `TARGET` argument is required.")
  endif ()
  if (NOT TARGET "${_vtk_standard_includes_TARGET}")
    message(FATAL_ERROR
      "The `TARGET` argument is not a target.")
  endif ()

  if (_vtk_standard_includes_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_standard_includes: "
      "${_vtk_standard_includes_UNPARSED_ARGUMENTS}")
  endif ()

  set(_vtk_standard_includes_system)
  if (_vtk_standard_includes_SYSTEM)
    set(_vtk_standard_includes_system SYSTEM)
  endif ()

  set(_vtk_standard_includes_visibility PUBLIC)
  if (_vtk_standard_includes_INTERFACE)
    set(_vtk_standard_includes_visibility INTERFACE)
  endif ()

  target_include_directories("${_vtk_standard_includes_TARGET}"
    ${_vtk_standard_includes_system}
    "${_vtk_standard_includes_visibility}"
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
      "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

  if (_vtk_build_INSTALL_HEADERS AND _vtk_standard_includes_HEADERS_DESTINATION AND NOT _vtk_add_module_NO_INSTALL)
    target_include_directories("${_vtk_standard_includes_TARGET}"
      ${_vtk_standard_includes_system}
      "${_vtk_standard_includes_visibility}"
      "$<INSTALL_INTERFACE:${_vtk_standard_includes_HEADERS_DESTINATION}>")
  endif ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_default_library_name

  Determine the default export macro for a module. |module-impl|

  Determines the export macro to be used for a module from its metadata. Assumes
  it is called from within a :cmake:command:`vtk_module_build call`.

  .. code-block:: cmake

    _vtk_module_default_library_name(<varname>)
#]==]
function (_vtk_module_default_export_macro_prefix varname)
  get_property(_vtk_module_default_library_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_library_name")
  string(TOUPPER "${_vtk_module_default_library_name}" _vtk_default_export_macro_upper)
  set("${varname}"
    "${_vtk_default_export_macro_upper}"
    PARENT_SCOPE)
endfunction ()

# TODO: It would be nice to support `USE_LINK_PROPERTIES` instead of listing
# the modules again here. However, the format of the `LINK_LIBRARIES` property
# value may not be easy to handle.

#[==[.rst:
.. _module-autoinit:

Autoinit
========

|module|

When a module contains a factory which may be populated by other modules, these
factories need to be populated when the modules are loaded by the dynamic linker
(for shared builds) or program load time (for static builds). To provide for
this, the module system contains an autoinit "subsystem".

.. _module-autoinit-leverage:

Leveraging the autoinit subsystem
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The subsystem provides the following hooks for use by projects:

* In modules which ``IMPLEMENTS`` other modules, in the generated
  ``<module>Module.h`` header (which provides export symbols as well) will
  include the modules which are implemented.
* In modules which are ``IMPLEMENTABLE`` or ``IMPLEMENTS`` another module, the
  generated ``<module>Module.h`` file will include the following block:

.. code-block:: c

  #ifdef <module>_AUTOINIT_INCLUDE
  #include <module>_AUTOINIT_INCLUDE
  #endif
  #ifdef <module>_AUTOINIT
  #include <header>
  VTK_MODULE_AUTOINIT(<module>)
  #endif

The :cmake:command:`vtk_module_autoinit` function will generate an include file and provide
its path via the ``<module>_AUTOINIT_INCLUDE`` define. once it has been included,
if the ``<module>_AUTOINIT`` symbol is defined, a header is included which is
intended to provide the ``VTK_MODULE_AUTOINIT`` macro. This macro is given the
module name and should use ``<module>_AUTOINIT`` to fill in the factories in the
module with those from the ``IMPLEMENTS`` modules listed in that symbol.

The ``<module>_AUTOINIT`` symbol's value is:

.. code-block::

  <count>(<module1>,<module2>,<module3>)

where ``<count>`` is the number of modules in the parentheses and each module
listed need to register something to ``<module>``.

If not provided via the ``AUTOINIT_INCLUDE`` argument to the
:cmake:command:`vtk_module_add_module` function, the header to use is fetched from the
``_vtk_module_autoinit_include`` global property. This only needs to be managed
in modules that ``IMPLEMENTS`` or are ``IMPLEMENTABLE``. This should be provided by
projects using the module system at its lowest level. Projects not implementing
the ``VTK_MODULE_AUTOINIT`` macro should have its value provided by
``find_package`` dependencies in some way.
#]==]

#[==[.rst:
.. cmake:command:: vtk_module_autoinit

  Linking to autoinit-using modules. |module|

  When linking to modules, in order for the autoinit system to work, modules need
  to declare their registration. In order to do this, defines may need to be
  provided to targets in order to trigger registration. These defines may be
  added to targets by using this function.

  .. code-block:: cmake

    vtk_module_autoinit(
      TARGETS <target>...
      MODULES <module>...)

  After this call, the targets given to the ``TARGETS`` argument will gain the
  preprocessor definitions to trigger registrations properly.
#]==]
function (vtk_module_autoinit)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_autoinit
    ""
    ""
    "TARGETS;MODULES")

  if (_vtk_autoinit_UNRECOGNIZED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_autoinit: "
      "${_vtk_autoinit_UNRECOGNIZED_ARGUMENTS}.")
  endif ()

  if (NOT _vtk_autoinit_TARGETS)
    message(FATAL_ERROR
      "The `TARGETS` argument is required.")
  endif ()

  if (NOT _vtk_autoinit_MODULES)
    message(AUTHOR_WARNING
      "No `MODULES` passed to `vtk_modules_autoinit`.")
  endif ()

  set(_vtk_autoinit_module_stack
    ${_vtk_autoinit_MODULES})

  set(_vtk_autoinit_needs_implements)
  set(_vtk_autoinit_seen)
  while (_vtk_autoinit_module_stack)
    list(GET _vtk_autoinit_module_stack 0 _vtk_autoinit_current_module)
    list(REMOVE_AT _vtk_autoinit_module_stack 0)
    if (_vtk_autoinit_current_module IN_LIST _vtk_autoinit_seen)
      continue ()
    endif ()
    list(APPEND _vtk_autoinit_seen
      "${_vtk_autoinit_current_module}")

    _vtk_module_real_target(_vtk_autoinit_current_target "${_vtk_autoinit_current_module}")
    get_property(_vtk_autoinit_implements
      TARGET    "${_vtk_autoinit_current_target}"
      PROPERTY  "INTERFACE_vtk_module_implements")

    list(APPEND _vtk_autoinit_needs_implements
      ${_vtk_autoinit_implements})
    foreach (_vtk_autoinit_implement IN LISTS _vtk_autoinit_implements)
      _vtk_module_real_target(_vtk_autoinit_implements_target "${_vtk_autoinit_implement}")
      get_property(_vtk_autoinit_implementable
        TARGET    "${_vtk_autoinit_implements_target}"
        PROPERTY  "INTERFACE_vtk_module_implementable")

      if (NOT _vtk_autoinit_implementable)
        message(FATAL_ERROR
          "The `${_vtk_autoinit_current_module}` module says that it "
          "implements the `${_vtk_autoinit_implement}` module, but it is not "
          "implementable.")
      endif ()

      list(APPEND "_vtk_autoinit_implements_${_vtk_autoinit_implement}"
        "${_vtk_autoinit_current_module}")
    endforeach ()
  endwhile ()

  if (NOT _vtk_autoinit_needs_implements)
    return ()
  endif ()
  list(REMOVE_DUPLICATES _vtk_autoinit_needs_implements)
  list(SORT _vtk_autoinit_needs_implements)

  set(_vtk_autoinit_hash_content)
  foreach (_vtk_autoinit_need_implements IN LISTS _vtk_autoinit_needs_implements)
    if (NOT _vtk_autoinit_implements_${_vtk_autoinit_need_implements})
      continue ()
    endif ()
    list(SORT "_vtk_autoinit_implements_${_vtk_autoinit_need_implements}")

    string(APPEND _vtk_autoinit_hash_content
      "${_vtk_autoinit_need_implements}: ${_vtk_autoinit_implements_${_vtk_autoinit_need_implements}}\n")
  endforeach ()
  string(MD5 _vtk_autoinit_header_tag "${_vtk_autoinit_hash_content}")
  set(_vtk_autoinit_header
    "${CMAKE_BINARY_DIR}/CMakeFiles/vtkModuleAutoInit_${_vtk_autoinit_header_tag}.h")

  get_property(_vtk_autoinit_header_generated GLOBAL
    PROPERTY "_vtk_autoinit_generated_${_vtk_autoinit_header_tag}")

  set(_vtk_autoinit_defines)
  set(_vtk_autoinit_header_content)
  foreach (_vtk_autoinit_need_implements IN LISTS _vtk_autoinit_needs_implements)
    if (NOT _vtk_autoinit_implements_${_vtk_autoinit_need_implements})
      continue ()
    endif ()

    get_property(_vtk_autoinit_implements_library_name
      TARGET    "${_vtk_autoinit_need_implements}"
      PROPERTY  "INTERFACE_vtk_module_library_name")

    if (NOT _vtk_autoinit_header_generated)
      list(LENGTH "_vtk_autoinit_implements_${_vtk_autoinit_need_implements}"
        _vtk_autoinit_length)
      set(_vtk_autoinit_args)
      foreach (_vtk_autoinit_arg IN LISTS "_vtk_autoinit_implements_${_vtk_autoinit_need_implements}")
        get_property(_vtk_autoinit_arg_library_name
          TARGET    "${_vtk_autoinit_arg}"
          PROPERTY  "INTERFACE_vtk_module_library_name")
        list(APPEND _vtk_autoinit_args
          "${_vtk_autoinit_arg_library_name}")
      endforeach ()
      string(REPLACE ";" "," _vtk_autoinit_args "${_vtk_autoinit_args}")
      string(APPEND _vtk_autoinit_header_content
        "#define ${_vtk_autoinit_implements_library_name}_AUTOINIT ${_vtk_autoinit_length}(${_vtk_autoinit_args})\n")
    endif ()

    list(APPEND _vtk_autoinit_defines
      "${_vtk_autoinit_implements_library_name}_AUTOINIT_INCLUDE=\"${_vtk_autoinit_header}\"")
  endforeach ()

  if (NOT _vtk_autoinit_header_generated)
    file(GENERATE
      OUTPUT  "${_vtk_autoinit_header}"
      CONTENT "${_vtk_autoinit_header_content}")

    set_property(GLOBAL
      PROPERTY
        "_vtk_autoinit_generated_${_vtk_autoinit_header_tag}" TRUE)
  endif ()

  foreach (_vtk_autoinit_target IN LISTS _vtk_autoinit_TARGETS)
    get_property(_vtk_autoinit_target_type
      TARGET    "${_vtk_autoinit_target}"
      PROPERTY  TYPE)
    if (_vtk_autoinit_target_type STREQUAL "INTERFACE_LIBRARY")
      continue ()
    endif ()

    target_compile_definitions("${_vtk_autoinit_target}"
      PRIVATE
        ${_vtk_autoinit_defines})
  endforeach ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_depfile_args

  Compute supported depfile tracking arguments. |module-internal|

  Support for ``add_custom_command(DEPFILE)`` has changed over the CMake
  timeline. Generate the required arguments as supported for the current CMake
  version and generator.

  .. code-block:: cmake

    _vtk_module_depfile_args(
      [MULTI_CONFIG_NEEDS_GENEX]
      TOOL_ARGS <variable>
      CUSTOM_COMMAND_ARGS <variable>
      DEPFILE_PATH <path>
      [SOURCE <path>]
      [SOURCE_LANGUAGE <lang>]
      [DEPFILE_NO_GENEX_PATH <path>]
      [TOOL_FLAGS <flag>...])

  The arguments to pass to the tool are returned in the variable given to
  ``TOOL_ARGS`` while the arguments for ``add_custom_command`` itself are
  returned in the variable given to ``CUSTOM_COMMAND_ARGS``. ``DEPFILE_PATH``
  is the path to the depfile to use. If a generator expression can optionally
  be used, ``DEPFILE_NO_GENEX_PATH`` can be specified as a fallback in case of
  no generator expression support (unless ``MULTI_CONFIG_NEEDS_GENEX`` is
  specified and a multi-config generator is used). ``TOOL_FLAGS`` specifies the
  flags the tool needs to specify the depfile if used. If support is not
  available, the path given to ``SOURCE`` is used for ``IMPLICIT_DEPENDS``
  using ``SOURCE_LANGUAGE`` (which defaults to ``CXX``).
#]==]
function (_vtk_module_depfile_args)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_depfile_args
    "MULTI_CONFIG_NEEDS_GENEX"
    "TOOL_ARGS;CUSTOM_COMMAND_ARGS;DEPFILE_PATH;DEPFILE_NO_GENEX_PATH;SOURCE;SOURCE_LANGUAGE"
    "TOOL_FLAGS")
  if (_vtk_depfile_args_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unrecognized arguments for _vtk_module_depfile_args: "
      "${_vtk_depfile_args_UNPARSED_ARGUMENTS}.")
  endif ()

  if (NOT _vtk_depfile_args_TOOL_ARGS)
    message(FATAL_ERROR
      "The `TOOL_ARGS` argument is required.")
  endif ()

  if (NOT _vtk_depfile_args_CUSTOM_COMMAND_ARGS)
    message(FATAL_ERROR
      "The `CUSTOM_COMMAND_ARGS` argument is required.")
  endif ()

  if (NOT _vtk_depfile_args_DEPFILE_PATH)
    message(FATAL_ERROR
      "The `DEPFILE_PATH` argument is required.")
  endif ()

  if (DEFINED _vtk_depfile_args_SOURCE_LANGUAGE AND
      NOT _vtk_depfile_args_SOURCE)
    message(FATAL_ERROR
      "Specifying `SOURCE_LANGUAGE` requires a `SOURCE` argument.")
  endif ()

  if (NOT DEFINED _vtk_depfile_args_SOURCE_LANGUAGE)
    set(_vtk_depfile_args_SOURCE_LANGUAGE "CXX")
  endif ()

  if (_vtk_depfile_args_DEPFILE_NO_GENEX_PATH AND
      _vtk_depfile_args_DEPFILE_NO_GENEX_PATH MATCHES "\\$<")
    message(FATAL_ERROR
      "The `DEPFILE_NO_GENEX_PATH` cannot contain a generator expression.")
  endif ()

  # Detect the required CMake version for `DEPFILE` support in the current
  # generator.
  if (CMAKE_GENERATOR STREQUAL "Ninja")
    set(_vtk_depfile_args_req_cmake "3.7")
  elseif (CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
    set(_vtk_depfile_args_req_cmake "3.17")
  elseif (CMAKE_GENERATOR MATCHES "Makefiles")
    set(_vtk_depfile_args_req_cmake "3.20")
  elseif (CMAKE_GENERATOR MATCHES "Xcode|Visual Studio")
    set(_vtk_depfile_args_req_cmake "3.21")
  else ()
    set(_vtk_depfile_args_req_cmake "99")
  endif ()

  # Check for generator expression requirements.
  set(_vtk_depfile_args_depfile_path "${_vtk_depfile_args_DEPFILE_PATH}")
  if (_vtk_depfile_args_req_cmake VERSION_LESS "3.21" AND
      _vtk_depfile_args_DEPFILE_PATH MATCHES "\\$<")
    get_property(_vtk_depfile_args_is_multi_config GLOBAL
      PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if (_vtk_depfile_args_DEPFILE_NO_GENEX_PATH AND
        NOT (_vtk_depfile_args_MULTI_CONFIG_NEEDS_GENEX AND
             _vtk_depfile_args_is_multi_config))
      set(_vtk_depfile_args_depfile_path "${_vtk_depfile_args_DEPFILE_NO_GENEX_PATH}")
    else ()
      set(_vtk_depfile_args_req_cmake "3.21")
    endif ()
  endif ()

  # Generate the arguments supported.
  if (CMAKE_VERSION VERSION_LESS _vtk_depfile_args_req_cmake)
    set(_vtk_depfile_args_tool_args)
    set(_vtk_depfile_args_custom_command_args)
    if (_vtk_depfile_args_SOURCE)
      list(APPEND _vtk_depfile_args_custom_command_args
        IMPLICIT_DEPENDS
          "${_vtk_depfile_args_SOURCE_LANGUAGE}" "${_vtk_depfile_args_SOURCE}")
    endif ()
  else ()
    set(_vtk_depfile_args_tool_args
      ${_vtk_depfile_args_TOOL_FLAGS}
      "${_vtk_depfile_args_depfile_path}")
    set(_vtk_depfile_args_custom_command_args
      DEPFILE "${_vtk_depfile_args_depfile_path}")
  endif ()

  # Return the computed arguments.
  set("${_vtk_depfile_args_TOOL_ARGS}"
    ${_vtk_depfile_args_tool_args}
    PARENT_SCOPE)
  set("${_vtk_depfile_args_CUSTOM_COMMAND_ARGS}"
    ${_vtk_depfile_args_custom_command_args}
    PARENT_SCOPE)
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_write_wrap_hierarchy

  Generate the hierarchy for a module. |module-impl|

  Write wrap hierarchy files for the module currently being built. This also
  installs the hierarchy file for use by dependent projects if ``INSTALL_HEADERS``
  is set. This function honors the ``HEADERS_COMPONENT``, and
  ``HEADERS_EXCLUDE_FROM_ALL`` arguments to :cmake:command:`vtk_module_build`.

  .. code-block:: cmake

    _vtk_module_write_wrap_hierarchy()
#]==]
function (_vtk_module_write_wrap_hierarchy)
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${_vtk_build_HIERARCHY_DESTINATION}")

  get_property(_vtk_hierarchy_library_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_library_name")
  set(_vtk_hierarchy_filename "${_vtk_hierarchy_library_name}-hierarchy.txt")
  set(_vtk_hierarchy_file "${CMAKE_BINARY_DIR}/${_vtk_build_HIERARCHY_DESTINATION}/${_vtk_hierarchy_filename}")
  set(_vtk_hierarchy_args_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_hierarchy_library_name}-hierarchy.$<CONFIGURATION>.args")
  set(_vtk_hierarchy_data_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_hierarchy_library_name}-hierarchy.data")
  set(_vtk_hierarchy_depends_args_file "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_hierarchy_library_name}-hierarchy.depends.args")
  set(_vtk_hierarchy_depfile_genex "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_hierarchy_filename}.$<CONFIGURATION>.d")
  set(_vtk_hierarchy_depfile_nogenex "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_hierarchy_filename}.d")
  _vtk_module_depfile_args(
    MULTI_CONFIG_NEEDS_GENEX
    TOOL_ARGS _vtk_hierarchy_depfile_flags
    CUSTOM_COMMAND_ARGS _vtk_hierarchy_depfile_args
    DEPFILE_PATH "${_vtk_hierarchy_depfile_genex}"
    DEPFILE_NO_GENEX_PATH "${_vtk_hierarchy_depfile_nogenex}"
    TOOL_FLAGS "-MF")

  set_property(TARGET "${_vtk_add_module_real_target}"
    PROPERTY
      "INTERFACE_vtk_module_hierarchy" "${_vtk_hierarchy_file}")

  set(_vtk_add_module_target_name_iface "${_vtk_add_module_target_name}")
  if (_vtk_add_module_build_with_kit)
    string(APPEND _vtk_add_module_target_name_iface "-objects")
  endif ()
  set(_vtk_hierarchy_genex_allowed 1)
  if (CMAKE_VERSION VERSION_LESS "3.19")
    get_property(_vtk_hierarchy_target_type
      TARGET   "${_vtk_add_module_real_target}"
      PROPERTY TYPE)
    if (_vtk_hierarchy_target_type STREQUAL "INTERFACE_LIBRARY")
      set(_vtk_hierarchy_genex_allowed 0)
    endif ()
  endif ()

  set(_vtk_hierarchy_genex_compile_definitions "")
  set(_vtk_hierarchy_genex_include_directories "")
  if (_vtk_hierarchy_genex_allowed)
    set(_vtk_hierarchy_genex_compile_definitions
      "$<TARGET_PROPERTY:${_vtk_add_module_target_name_iface},COMPILE_DEFINITIONS>")
    set(_vtk_hierarchy_genex_include_directories
      "$<TARGET_PROPERTY:${_vtk_add_module_target_name_iface},INCLUDE_DIRECTORIES>")
  else ()
    if (NOT DEFINED ENV{CI})
      message(AUTHOR_WARNING
        "Hierarchy generation is not using target-local compile definitions "
        "or include directories. This may affect generation of the hierarchy "
        "files for the ${_vtk_build_module} module. Use CMake 3.19+ to "
        "guarantee intended behavior.")
    endif ()
  endif ()
  file(GENERATE
    OUTPUT  "${_vtk_hierarchy_args_file}"
    CONTENT "$<$<BOOL:${_vtk_hierarchy_genex_compile_definitions}>:\n-D\'$<JOIN:${_vtk_hierarchy_genex_compile_definitions},\'\n-D\'>\'>\n
$<$<BOOL:${_vtk_hierarchy_genex_include_directories}>:\n-I\'$<JOIN:${_vtk_hierarchy_genex_include_directories},\'\n-I\'>\'>\n")

  get_property(_vtk_hierarchy_depends_is_global GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_module}_depends"
    SET)
  if (_vtk_hierarchy_depends_is_global)
    get_property(_vtk_hierarchy_depends GLOBAL
      PROPERTY  "_vtk_module_${_vtk_build_module}_depends")
  else ()
    get_property(_vtk_hierarchy_depends GLOBAL
      TARGET    "${_vtk_add_module_real_target}"
      PROPERTY  "INTERFACE_vtk_module_depends")
  endif ()

  set(_vtk_hierarchy_depends_files)
  set(_vtk_hierarchy_depends_targets)
  foreach (_vtk_hierarchy_depend IN LISTS _vtk_hierarchy_depends)
    _vtk_module_get_module_property("${_vtk_hierarchy_depend}"
      PROPERTY  "hierarchy"
      VARIABLE  _vtk_hierarchy_depend_hierarchy)
    if (NOT DEFINED _vtk_hierarchy_depend_hierarchy)
      continue ()
    endif ()

    list(APPEND _vtk_hierarchy_depends_files
      "${_vtk_hierarchy_depend_hierarchy}")

    # Find the hierarchy target of the module.
    get_property(_vtk_hierarchy_module_is_imported
      TARGET    "${_vtk_hierarchy_depend}"
      PROPERTY  IMPORTED)
    # Imported target modules are external and should already have their file
    # generated.
    if (_vtk_hierarchy_module_is_imported)
      continue ()
    endif ()

    get_property(_vtk_hierarchy_depend_library_name GLOBAL
      PROPERTY "_vtk_module_${_vtk_hierarchy_depend}_library_name")
    if (TARGET "${_vtk_hierarchy_depend_library_name}-hierarchy")
      list(APPEND _vtk_hierarchy_depends_targets
        "${_vtk_hierarchy_depend_library_name}-hierarchy")
    endif ()
  endforeach ()

  set(_vtk_hierarchy_depends_files_arg)
  if (_vtk_hierarchy_depends_files)
    file(GENERATE
      OUTPUT  "${_vtk_hierarchy_depends_args_file}"
      CONTENT "\"$<JOIN:${_vtk_hierarchy_depends_files},\"\n\">\"\n")
  else ()
    file(GENERATE
      OUTPUT  "${_vtk_hierarchy_depends_args_file}"
      CONTENT "")
  endif ()

  _vtk_module_get_module_property("${_vtk_build_module}"
    PROPERTY  "headers"
    VARIABLE  _vtk_hierarchy_headers)
  set(_vtk_hierarchy_data_content "")
  foreach (_vtk_hierarchy_header IN LISTS _vtk_hierarchy_headers)
    string(APPEND _vtk_hierarchy_data_content
      "${_vtk_hierarchy_header};${_vtk_hierarchy_library_name}\n")
  endforeach ()
  file(GENERATE
    OUTPUT  "${_vtk_hierarchy_data_file}"
    CONTENT "${_vtk_hierarchy_data_content}")

  if (CMAKE_GENERATOR MATCHES "Ninja")
    set(_vtk_hierarchy_command_depends ${_vtk_hierarchy_depends_files})
  else ()
    set(_vtk_hierarchy_command_depends ${_vtk_hierarchy_depends_targets})
  endif ()

  set(_vtk_hierarchy_tool_target "VTK::WrapHierarchy")
  set(_vtk_hierarchy_macros_args)
  if (TARGET VTKCompileTools::WrapHierarchy)
    set(_vtk_hierarchy_tool_target "VTKCompileTools::WrapHierarchy")
    if (TARGET VTKCompileTools_macros)
      list(APPEND _vtk_hierarchy_command_depends
        "VTKCompileTools_macros")
      list(APPEND _vtk_hierarchy_macros_args
        -undef
        -imacros "${_VTKCompileTools_macros_file}")
    endif ()
  endif ()

  cmake_policy(PUSH)
  if(POLICY CMP0116)
    cmake_policy(SET CMP0116 NEW) # DEPFILE argument is relative to CMAKE_CURRENT_BINARY_DIR
  endif()
  add_custom_command(
    OUTPUT  "${_vtk_hierarchy_file}"
    COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR}
            "$<TARGET_FILE:${_vtk_hierarchy_tool_target}>"
            ${_vtk_hierarchy_depfile_flags}
            "@${_vtk_hierarchy_args_file}"
            -o "${_vtk_hierarchy_file}"
            "${_vtk_hierarchy_data_file}"
            "@${_vtk_hierarchy_depends_args_file}"
            ${_vtk_hierarchy_macros_args}
    ${_vtk_hierarchy_depfile_args}
    COMMENT "Generating the wrap hierarchy for ${_vtk_build_module}"
    DEPENDS
      ${_vtk_hierarchy_headers}
      "${_vtk_hierarchy_args_file}"
      "${_vtk_hierarchy_data_file}"
      "${_vtk_hierarchy_depends_args_file}"
      ${_vtk_hierarchy_command_depends})
  cmake_policy(POP)
  add_custom_target("${_vtk_add_module_library_name}-hierarchy" ALL
    DEPENDS
      "${_vtk_hierarchy_file}"
      "$<TARGET_FILE:${_vtk_hierarchy_tool_target}>")
  set_property(TARGET "${_vtk_add_module_real_target}"
    PROPERTY
      "INTERFACE_vtk_module_hierarchy" "${_vtk_hierarchy_file}")

  if (_vtk_build_INSTALL_HEADERS)
    set(_vtk_hierarchy_headers_component "${_vtk_build_HEADERS_COMPONENT}")
    if (_vtk_build_TARGET_SPECIFIC_COMPONENTS)
      string(PREPEND _vtk_hierarchy_headers_component "${_vtk_build_module}-")
      if (_vtk_build_BUILD_WITH_KITS)
        get_property(_vtk_hierarchy_build_with_kit GLOBAL
          PROPERTY "_vtk_module_${_vtk_build_module}_kit")
        if (_vtk_hierarchy_build_with_kit)
          set(_vtk_hierarchy_headers_component "${_vtk_hierarchy_build_with_kit}-${_vtk_build_HEADERS_COMPONENT}")
        endif ()
      endif ()
    endif ()
    set_property(TARGET "${_vtk_add_module_real_target}"
      PROPERTY
        "INTERFACE_vtk_module_hierarchy_install" "\${_vtk_module_import_prefix}/${_vtk_build_HIERARCHY_DESTINATION}/${_vtk_hierarchy_filename}")

    set(_vtk_hierarchy_exclude_from_all "")
    if (_vtk_build_HEADERS_EXCLUDE_FROM_ALL)
      set(_vtk_hierarchy_exclude_from_all "EXCLUDE_FROM_ALL")
    endif ()

    install(
      FILES       "${_vtk_hierarchy_file}"
      DESTINATION "${_vtk_build_HIERARCHY_DESTINATION}"
      RENAME      "${_vtk_hierarchy_filename}"
      COMPONENT   "${_vtk_hierarchy_headers_component}"
      ${_vtk_hierarchy_exclude_from_all})
  endif ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_add_file_set

  Add a file set to a target. |module-internal|

  .. code-block:: cmake

    _vtk_module_add_file_set(<target>
      NAME <name>
      [VIS <visibility>]
      [TYPE <type>]
      [BASE_DIRS <base directory>...]
      FILES
        [members...])

  Add a file set to the ``<target>`` named ``<name>``.

  * ``NAME``: The name of the file set.
  * ``VIS``: The visibility of the file set. Defaults to ``PRIVATE``.
    Must be a valid CMake visibility (``PUBLIC``, ``PRIVATE``, or
    ``INTERFACE``).
  * ``TYPE``: The type of the file set. Defaults to ``HEADERS``. File sets
    types that are recognized and known to not be supported by the CMake
    version in use will be added as ``PRIVATE`` sources not part of any file
    set.
  * ``BASE_DIRS``: Base directories for the files. Defaults to
    ``${CMAKE_CURRENT_SOURCE_DIR}`` and ``${CMAKE_CURRENT_BINARY_DIR}`` if not
    specified.
  * ``FILES``: The paths to add to the file set.

  Note that prior to CMake 3.19, usage of ``FILE_SET`` with ``INTERFACE``
  targets is severely restricted and instead this function will do nothing. Any
  ``PUBLIC`` files specified this way need installed using standard mechanisms.

#]==]
function (_vtk_module_add_file_set target)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_add_file_set
    ""
    "NAME;VIS;TYPE"
    "BASE_DIRS;FILES")

  if (_vtk_add_file_set_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for _vtk_module_add_file_set: "
      "${_vtk_add_file_set_UNPARSED_ARGUMENTS}")
  endif ()

  set(_vtk_add_file_set_known_visibilities
    PRIVATE
    PUBLIC
    INTERFACE)
  if (NOT DEFINED _vtk_add_file_set_VIS)
    set(_vtk_add_file_set_VIS
      PRIVATE)
  endif ()
  if (NOT _vtk_add_file_set_VIS IN_LIST _vtk_add_file_set_known_visibilities)
    string(REPLACE ";" ", " _vtk_add_file_set_known_list "${_vtk_add_file_set_known_visibilities}")
    message(FATAL_ERROR
      "Unknown visibility '${_vtk_add_file_set_VIS}'. Must be one of "
      "${_vtk_add_file_set_known_list}")
  endif ()

  if (NOT DEFINED _vtk_add_file_set_TYPE)
    set(_vtk_add_file_set_TYPE "HEADERS")
  endif ()

  if (NOT DEFINED _vtk_add_file_set_BASE_DIRS)
    set(_vtk_add_file_set_BASE_DIRS
      "${CMAKE_CURRENT_SOURCE_DIR}"
      "${CMAKE_CURRENT_BINARY_DIR}")
  endif ()

  if ((DEFINED _vtk_build_USE_FILE_SETS AND NOT _vtk_build_USE_FILE_SETS) OR
      CMAKE_VERSION VERSION_LESS "3.23")
    # XXX(cmake-3.19): Using a non-`INTERFACE` `FILE_SET`s with `INTERFACE`
    # targets was added in CMake 3.19.
    if (CMAKE_VERSION VERSION_LESS "3.19")
      get_property(_vtk_add_file_set_type
        TARGET    "${target}"
        PROPERTY  TYPE)
      if (_vtk_add_file_set_type STREQUAL "INTERFACE_LIBRARY")
        return ()
      endif ()
    endif ()
    target_sources("${target}"
      PRIVATE
        ${_vtk_add_file_set_FILES})
    return ()
  endif ()

  target_sources("${target}"
    "${_vtk_add_file_set_VIS}"
      FILE_SET "${_vtk_add_file_set_NAME}"
        TYPE      "${_vtk_add_file_set_TYPE}"
        BASE_DIRS ${_vtk_add_file_set_BASE_DIRS}
        FILES     ${_vtk_add_file_set_FILES})
endfunction ()

include(GenerateExportHeader)
include("${CMAKE_CURRENT_LIST_DIR}/vtkModuleSerialization.cmake")

#[==[.rst:
.. cmake:command:: vtk_module_add_module

  Create a module library. |module|

  .. code-block:: cmake

     vtk_module_add_module(<name>
       [NO_INSTALL] [FORCE_STATIC|FORCE_SHARED]
       [HEADER_ONLY] [HEADER_DIRECTORIES]
       [EXPORT_MACRO_PREFIX      <prefix>]
       [HEADERS_SUBDIR           <subdir>]
       [LIBRARY_NAME_SUFFIX      <suffix>]
       [CLASSES                  <class>...]
       [TEMPLATE_CLASSES         <template class>...]
       [NOWRAP_CLASSES           <nowrap class>...]
       [NOWRAP_TEMPLATE_CLASSES  <nowrap template class>...]
       [SOURCES                  <source>...]
       [HEADERS                  <header>...]
       [NOWRAP_HEADERS           <header>...]
       [TEMPLATES                <template>...]
       [PRIVATE_CLASSES          <class>...]
       [PRIVATE_TEMPLATE_CLASSES <template class>...]
       [PRIVATE_HEADERS          <header>...]
       [PRIVATE_TEMPLATES        <template>...]
       [SPDX_SKIP_REGEX          <regex>])

  The ``PRIVATE_`` arguments are analogous to their non-``PRIVATE_`` arguments, but
  the associated files are not installed or available for wrapping (``SOURCES`` are
  always private, so there is no ``PRIVATE_`` variant for that argument).

  * ``NO_INSTALL``: Skip installation of the module and all its installation
    artifacts. Note that if this target is used by any other target that is
    exported, this option may not be used because CMake (in addition to VTK
    module APIs such as ``vtk_module_export_find_packages`` and ' ') will generate
    references to the target that are expected to be satisfied. It is highly
    recommended to test that the build and install exports (as used) be tested
    to make sure that the module is not actually referenced.
  * ``FORCE_STATIC`` or ``FORCE_SHARED``: For a static (respectively, shared)
    library to be created. If neither is provided, ``BUILD_SHARED_LIBS`` will
    control the library type.
  * ``HEADER_ONLY``: The module only contains headers (or templates) and contains
    no compilation steps. Mutually exclusive with ``FORCE_STATIC``.
  * ``HEADER_DIRECTORIES``: The headers for this module are in a directory
    structure which should be preserved in the install tree.
  * ``EXPORT_MACRO_PREFIX``: The prefix for the export macro definitions.
    Defaults to the library name of the module in all uppercase.
  * ``HEADERS_SUBDIR``: The subdirectory to install headers into in the install
    tree.
  * ``LIBRARY_NAME_SUFFIX``: The suffix to the module's library name if
    additional information is required.
  * ``CLASSES``: A list of classes in the module. This is a shortcut for adding
    ``<class>.cxx`` to ``SOURCES`` and ``<class>.h`` to ``HEADERS``.
  * ``TEMPLATE_CLASSES``: A list of template classes in the module. This is a
    shortcut for adding ``<class>.txx`` to ``TEMPLATES`` and ``<class>.h`` to
    ``HEADERS``.
  * ``SOURCES``: A list of source files which require compilation.
  * ``HEADERS``: A list of header files which will be available for wrapping and
    installed.
  * ``NOWRAP_CLASSES``: A list of classes which will not be available for
    wrapping but installed. This is a shortcut for adding ``<class>.cxx`` to
    ``SOURCES`` and ``<class>.h`` to ``NOWRAP_HEADERS``.
  * ``NOWRAP_TEMPLATE_CLASSES``: A list of template classes which will not be
  * available for
    wrapping but installed. This is a shortcut for adding ``<class>.txx`` to
    ``TEMPLATES`` and ``<class>.h`` to ``NOWRAP_HEADERS``.
  * ``NOWRAP_HEADERS``: A list of header files which will not be available for
    wrapping but installed.
  * ``TEMPLATES``: A list of template files which will be installed.
  * ``SPDX_SKIP_REGEX``: A python regex to skip a file based on its name
      when parsing for SPDX headers.
#]==]
function (vtk_module_add_module name)
  if (NOT name STREQUAL _vtk_build_module)
    message(FATAL_ERROR
      "The ${_vtk_build_module}'s CMakeLists.txt may not add the ${name} module.")
  endif ()

  set(_vtk_add_module_source_keywords)
  foreach (_vtk_add_module_kind IN ITEMS CLASSES TEMPLATE_CLASSES HEADERS TEMPLATES)
    list(APPEND _vtk_add_module_source_keywords
      "${_vtk_add_module_kind}"
      "PRIVATE_${_vtk_add_module_kind}")
  endforeach ()

  cmake_parse_arguments(PARSE_ARGV 1 _vtk_add_module
    "NO_INSTALL;FORCE_STATIC;FORCE_SHARED;HEADER_ONLY;HEADER_DIRECTORIES;EXCLUDE_HEADER_TEST"
    "EXPORT_MACRO_PREFIX;HEADERS_SUBDIR;LIBRARY_NAME_SUFFIX;SPDX_SKIP_REGEX"
    "${_vtk_add_module_source_keywords};SOURCES;NOWRAP_CLASSES;NOWRAP_TEMPLATE_CLASSES;NOWRAP_HEADERS")

  if (_vtk_add_module_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_add_module: "
      "${_vtk_add_module_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_add_module_EXPORT_MACRO_PREFIX)
    _vtk_module_default_export_macro_prefix(_vtk_add_module_EXPORT_MACRO_PREFIX)
  endif ()

  if (_vtk_add_module_HEADER_ONLY AND _vtk_add_module_FORCE_STATIC)
    message(FATAL_ERROR
      "The ${_vtk_build_module} module cannot be header only yet forced "
      "static.")
  endif ()

  if (_vtk_add_module_FORCE_SHARED AND _vtk_add_module_FORCE_STATIC)
    message(FATAL_ERROR
      "The ${_vtk_build_module} module cannot be both shared and static.")
  endif ()

  foreach (_vtk_add_module_class IN LISTS _vtk_add_module_CLASSES)
    list(APPEND _vtk_add_module_SOURCES
      "${_vtk_add_module_class}.cxx")
    list(APPEND _vtk_add_module_HEADERS
      "${_vtk_add_module_class}.h")
  endforeach ()

  foreach (_vtk_add_module_template_class IN LISTS _vtk_add_module_TEMPLATE_CLASSES)
    list(APPEND _vtk_add_module_TEMPLATES
      "${_vtk_add_module_template_class}.txx")
    list(APPEND _vtk_add_module_HEADERS
      "${_vtk_add_module_template_class}.h")
  endforeach ()

  foreach (_vtk_add_module_class IN LISTS _vtk_add_module_NOWRAP_CLASSES)
    list(APPEND _vtk_add_module_SOURCES
      "${_vtk_add_module_class}.cxx")
    list(APPEND _vtk_add_module_NOWRAP_HEADERS
      "${_vtk_add_module_class}.h")
  endforeach ()

  foreach (_vtk_add_module_class IN LISTS _vtk_add_module_NOWRAP_TEMPLATE_CLASSES)
    list(APPEND _vtk_add_module_TEMPLATES
      "${_vtk_add_module_class}.txx")
    list(APPEND _vtk_add_module_NOWRAP_HEADERS
      "${_vtk_add_module_class}.h")
  endforeach ()

  foreach (_vtk_add_module_class IN LISTS _vtk_add_module_PRIVATE_CLASSES)
    list(APPEND _vtk_add_module_SOURCES
      "${_vtk_add_module_class}.cxx")
    list(APPEND _vtk_add_module_PRIVATE_HEADERS
      "${_vtk_add_module_class}.h")
  endforeach ()

  foreach (_vtk_add_module_template_class IN LISTS _vtk_add_module_PRIVATE_TEMPLATE_CLASSES)
    list(APPEND _vtk_add_module_PRIVATE_TEMPLATES
      "${_vtk_add_module_template_class}.txx")
    list(APPEND _vtk_add_module_PRIVATE_HEADERS
      "${_vtk_add_module_template_class}.h")
  endforeach ()

  if (NOT _vtk_add_module_SOURCES AND NOT _vtk_add_module_HEADER_ONLY)
    message(AUTHOR_WARNING
      "The ${_vtk_build_module} module has no source files. Did you mean to "
      "pass the `HEADER_ONLY` flag?")
  endif ()

  get_property(_vtk_add_module_third_party GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_module}_third_party")

  get_property(_vtk_add_module_library_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_library_name")
  set(_vtk_add_module_module_header_name
    "${_vtk_add_module_library_name}Module.h")
  if (NOT _vtk_add_module_HEADER_ONLY AND NOT _vtk_add_module_third_party)
    set(_vtk_add_module_generated_header
      "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_add_module_module_header_name}")
    list(APPEND _vtk_add_module_NOWRAP_HEADERS
      "${_vtk_add_module_generated_header}")
  endif ()

  get_property(_vtk_add_module_include_marshal GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_module}_include_marshal")
  if (_vtk_build_ENABLE_SERIALIZATION AND _vtk_add_module_include_marshal)
    set(_vtk_add_module_serdes_registrar_header_name
      "${_vtk_add_module_library_name}SerDes.h")
    set(_vtk_add_module_serdes_registrar_source_name
      "${_vtk_add_module_library_name}SerDes.cxx")
    set(_vtk_add_module_serdes_registrar_header
      "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_add_module_serdes_registrar_header_name}")
    set(_vtk_add_module_serdes_registrar_source
      "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_add_module_serdes_registrar_source_name}")
    list(APPEND _vtk_add_module_NOWRAP_HEADERS
      "${_vtk_add_module_serdes_registrar_header}")
    list(APPEND _vtk_add_module_SOURCES
      "${_vtk_add_module_serdes_registrar_source}")
  endif ()

  set(_vtk_add_module_use_relative_paths)
  if (_vtk_add_module_HEADER_DIRECTORIES)
    set(_vtk_add_module_use_relative_paths
      USE_RELATIVE_PATHS)
  endif ()

  if (NOT _vtk_add_module_NO_INSTALL)
    # Warn if `HEADER_DIRECTORIES` is not specified and directories are found
    # in the source listings. But ignore third party packages; they have other
    # mechanisms.
    get_property(_vtk_add_module_is_third_party GLOBAL
      PROPERTY  "_vtk_module_${_vtk_build_module}_third_party")
    if (NOT _vtk_add_module_HEADER_DIRECTORIES)
      foreach (_vtk_add_module_header_path IN LISTS _vtk_add_module_HEADERS _vtk_add_module_NOWRAP_HEADERS _vtk_add_module_TEMPLATES)
        set(_vtk_add_module_header_path_orig "${_vtk_add_module_header_path}")
        set(_vtk_add_module_header_path_base "source")
        if (IS_ABSOLUTE "${_vtk_add_module_header_path}")
          file(RELATIVE_PATH _vtk_add_module_header_path
            "${CMAKE_CURRENT_BINARY_DIR}"
            "${_vtk_add_module_header_path}")
          if (_vtk_add_module_header_path MATCHES "^\\.\\./")
            message(AUTHOR_WARNING
              "The `${_vtk_add_module_header_path_orig}` source appears to "
              "be outside of the module's source or binary directory. "
              "Please relocate the source to be underneath one of the two "
              "directories.")
            continue ()
          endif ()
          set(_vtk_add_module_header_path_base "binary")
        endif ()
        if (_vtk_add_module_header_path MATCHES "/")
          message(AUTHOR_WARNING
            "The `${_vtk_add_module_header_path_orig}` source contains a "
            "directory between the ${_vtk_add_module_header_path_base} "
            "directory and its location. Newer CMake prefers to use "
            "`FILE_SET` installation which will keep this directory "
            "structure within the install tree. This module likely has a "
            "custom include directory setting to support consuming this "
            "header. Please update to support `HEADER_DIRECTORIES` as "
            "needed.")
        endif ()
      endforeach ()
    endif ()

    # XXX(cmake-3.23): file sets
    if (NOT _vtk_build_USE_FILE_SETS OR
        CMAKE_VERSION VERSION_LESS "3.23" OR
        # XXX(cmake-3.19): Using a non-`INTERFACE` `FILE_SET`s with `INTERFACE`
        # targets is not yet supported.
        (CMAKE_VERSION VERSION_LESS "3.19" AND
         _vtk_add_module_HEADER_ONLY))
      vtk_module_install_headers(
        ${_vtk_add_module_use_relative_paths}
        FILES   ${_vtk_add_module_HEADERS}
                ${_vtk_add_module_NOWRAP_HEADERS}
                ${_vtk_add_module_TEMPLATES}
        SUBDIR  "${_vtk_add_module_HEADERS_SUBDIR}")
    endif ()
  endif ()

  set(_vtk_add_module_type)
  if (_vtk_add_module_FORCE_STATIC)
    set(_vtk_add_module_type STATIC)
  elseif (_vtk_add_module_FORCE_SHARED)
    get_cmake_property(_vtk_add_module_target_has_shared TARGET_SUPPORTS_SHARED_LIBS)
    if (_vtk_add_module_target_has_shared)
      set(_vtk_add_module_type SHARED)
    else ()
      # XXX(cmake): Until the emscripten platform supports shared libraries,
      #   do not allow FORCE_SHARED. See [1] and [2] for more information.
      #   [1]: https://github.com/emscripten-core/emscripten/pull/16281
      #   [2]: https://github.com/emscripten-core/emscripten/issues/20340
      #
      #   If some *other* platform does not support shared libraries, issue a warning:
      if (NOT EMSCRIPTEN)
        message(WARNING "Shared library requested by ${name} not allowed on this platform.")
      endif ()
    endif ()
  endif ()

  set(_vtk_add_module_build_with_kit)
  if (_vtk_build_BUILD_WITH_KITS)
    get_property(_vtk_add_module_build_with_kit GLOBAL
      PROPERTY "_vtk_module_${_vtk_build_module}_kit")
  endif ()

  get_property(_vtk_add_module_namespace GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_module}_namespace")
  get_property(_vtk_add_module_target_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_target_name")
  set(_vtk_add_module_real_target "${_vtk_add_module_target_name}")
  if (_vtk_add_module_HEADER_ONLY)
    if (_vtk_add_module_build_with_kit)
      message(FATAL_ERROR
        "The module ${_vtk_build_module} is header-only, but is part of the "
        "${_vtk_add_module_build_with_kit} kit. Header-only modules do not "
        "belong in kits.")
    endif ()

    add_library("${_vtk_add_module_real_target}" INTERFACE)
    _vtk_module_add_file_set("${_vtk_add_module_real_target}"
      NAME  vtk_module_templates
      VIS   PUBLIC
      FILES ${_vtk_add_module_TEMPLATES})
    _vtk_module_add_file_set("${_vtk_add_module_real_target}"
      NAME  vtk_module_headers
      VIS   PUBLIC
      FILES ${_vtk_add_module_HEADERS})
    _vtk_module_add_file_set("${_vtk_add_module_real_target}"
      NAME  vtk_module_nowrap_headers
      VIS   PUBLIC
      FILES ${_vtk_add_module_NOWRAP_HEADERS})

    if (NOT _vtk_build_module STREQUAL _vtk_add_module_real_target)
      add_library("${_vtk_build_module}" ALIAS
        "${_vtk_add_module_real_target}")
    endif ()
  else ()
    if (_vtk_add_module_build_with_kit)
      add_library("${_vtk_add_module_real_target}" INTERFACE)
      target_link_libraries("${_vtk_add_module_real_target}"
        INTERFACE
          # For usage requirements.
          "${_vtk_add_module_real_target}-objects"
          # For the implementation.
          "$<LINK_ONLY:${_vtk_add_module_build_with_kit}>")

      if (NOT _vtk_build_module STREQUAL _vtk_add_module_real_target)
        add_library("${_vtk_build_module}" ALIAS
          "${_vtk_add_module_real_target}")
      endif ()

      # Set up properties necessary for other infrastructure.
      set_property(TARGET "${_vtk_add_module_real_target}"
        PROPERTY
          "INTERFACE_vtk_module_library_name" "${_vtk_add_module_library_name}")

      add_library("${_vtk_add_module_real_target}-objects" OBJECT)

      set_property(TARGET "${_vtk_add_module_real_target}-objects"
        PROPERTY
          # Emulate the regular library as much as possible.
          DEFINE_SYMBOL             "${_vtk_add_module_real_target}_EXPORT")
      target_compile_definitions("${_vtk_add_module_real_target}-objects"
        PRIVATE
          "${_vtk_add_module_real_target}_EXPORT")
      string(APPEND _vtk_add_module_real_target "-objects")
    else ()
      add_library("${_vtk_add_module_real_target}" ${_vtk_add_module_type})

      if (NOT _vtk_build_module STREQUAL _vtk_add_module_real_target)
        add_library("${_vtk_build_module}" ALIAS
          "${_vtk_add_module_real_target}")
      endif ()
    endif ()

    target_sources("${_vtk_add_module_real_target}"
      PRIVATE
        ${_vtk_add_module_SOURCES})
    _vtk_module_add_file_set("${_vtk_add_module_real_target}"
      NAME  vtk_module_private_templates
      FILES ${_vtk_add_module_PRIVATE_TEMPLATES})
    _vtk_module_add_file_set("${_vtk_add_module_real_target}"
      NAME  vtk_module_templates
      VIS   PUBLIC
      FILES ${_vtk_add_module_TEMPLATES})
    _vtk_module_add_file_set("${_vtk_add_module_real_target}"
      NAME  vtk_module_private_headers
      FILES ${_vtk_add_module_PRIVATE_HEADERS})
    _vtk_module_add_file_set("${_vtk_add_module_real_target}"
      NAME  vtk_module_headers
      VIS   PUBLIC
      FILES ${_vtk_add_module_HEADERS})
    _vtk_module_add_file_set("${_vtk_add_module_real_target}"
      NAME  vtk_module_nowrap_headers
      VIS   PUBLIC
      FILES ${_vtk_add_module_NOWRAP_HEADERS})

    if (_vtk_build_UTILITY_TARGET)
      target_link_libraries("${_vtk_add_module_real_target}"
        PRIVATE
          "${_vtk_build_UTILITY_TARGET}")
    endif ()

    set_property(TARGET "${_vtk_add_module_real_target}"
      PROPERTY
        POSITION_INDEPENDENT_CODE ON)
  endif ()

  set_property(TARGET "${_vtk_add_module_real_target}"
    PROPERTY
      "INTERFACE_vtk_module_library_name" "${_vtk_add_module_library_name}")

  get_property(_vtk_add_module_depends GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_module}_depends")
  set_property(TARGET "${_vtk_add_module_real_target}"
    PROPERTY
      "INTERFACE_vtk_module_depends" "${_vtk_add_module_depends}")
  set(_vtk_add_module_includes_interface)
  if (_vtk_add_module_HEADER_ONLY)
    target_link_libraries("${_vtk_add_module_real_target}"
      INTERFACE
        ${_vtk_add_module_depends})
    set(_vtk_add_module_includes_interface INTERFACE)
  else ()
    get_property(_vtk_add_module_private_depends GLOBAL
      PROPERTY  "_vtk_module_${_vtk_build_module}_private_depends")

    # XXX(cmake#18484): Linking dependencies directly currently creates
    # circular dependencies. This logic should be removed once the minimum for
    # kits contains a fix for the mentioned issue.
    #
    # When two modules are part of the same kit, we can get this problem:
    #
    #   A - iface -> A-objects <- tll - K
    #   ^                               |
    #   |                               |
    #   B - iface -> B-objects <- tll -/
    #
    # If B depends on A, it ends up with a circular dependency since A has a
    # `$<LINK_ONLY:K>` link. instead, munge up dependencies of intra-kit
    # dependencies to link to the `-objects` target instead.
    if (_vtk_add_module_build_with_kit)
      set(_vtk_add_module_depends_link)
      set(_vtk_add_module_private_depends_link)
      foreach (_vtk_add_module_depend IN LISTS _vtk_add_module_depends)
        get_property(_vtk_add_module_depend_kit GLOBAL
          PROPERTY "_vtk_module_${_vtk_add_module_depend}_kit")
        if (_vtk_add_module_depend_kit STREQUAL _vtk_add_module_build_with_kit)
          # We're in the same kit; depend on the `-objects` library of the
          # module.
          get_property(_vtk_add_module_depend_target_name GLOBAL
            PROPERTY "_vtk_module_${_vtk_add_module_depend}_target_name")
          list(APPEND _vtk_add_module_depends_link
            "${_vtk_add_module_depend_target_name}-objects")
        else ()
          # Different kit, just use as normal.
          list(APPEND _vtk_add_module_depends_link
            "${_vtk_add_module_depend}")
        endif ()
      endforeach ()
      foreach (_vtk_add_module_private_depend IN LISTS _vtk_add_module_private_depends)
        get_property(_vtk_add_module_private_depend_kit GLOBAL
          PROPERTY "_vtk_module_${_vtk_add_module_private_depend}_kit")
        if (_vtk_add_module_private_depend_kit STREQUAL _vtk_add_module_build_with_kit)
          # We're in the same kit; depend on the `-objects` library of the
          # module.
          get_property(_vtk_add_module_private_depend_target_name GLOBAL
            PROPERTY "_vtk_module_${_vtk_add_module_private_depend}_target_name")
          list(APPEND _vtk_add_module_private_depends_link
            "${_vtk_add_module_private_depend_target_name}-objects")
        else ()
          # Different kit, just use as normal.
          list(APPEND _vtk_add_module_private_depends_link
            "${_vtk_add_module_private_depend}")
        endif ()
      endforeach ()

      # Add the `DEFINE_SYMBOL` for all other modules within the same kit which
      # have already been processed because the direct dependencies are not
      # sufficient: export symbols from any included header needs to be
      # correct. Since modules are built in topological order, a module can
      # only possibly include modules in the kit which have already been built.
      get_property(_vtk_add_module_kit_modules GLOBAL
        PROPERTY  "_vtk_kit_${_vtk_add_module_build_with_kit}_kit_modules")
      list(REMOVE_ITEM _vtk_add_module_kit_modules "${_vtk_build_module}")
      foreach (_vtk_add_module_kit_module IN LISTS _vtk_add_module_kit_modules)
        get_property(_vtk_add_module_kit_module_target_name GLOBAL
          PROPERTY "_vtk_module_${_vtk_add_module_kit_module}_target_name")
        if (TARGET "${_vtk_add_module_kit_module_target_name}-objects")
          get_property(_vtk_add_module_kit_module_define_symbol
            TARGET    "${_vtk_add_module_kit_module_target_name}-objects"
            PROPERTY  DEFINE_SYMBOL)
          target_compile_definitions("${_vtk_add_module_real_target}"
            PRIVATE
              "${_vtk_add_module_kit_module_define_symbol}")
        endif ()
      endforeach ()
    else ()
      set(_vtk_add_module_depends_link ${_vtk_add_module_depends})
      set(_vtk_add_module_private_depends_link ${_vtk_add_module_private_depends})
    endif ()
    target_link_libraries("${_vtk_add_module_real_target}"
      PUBLIC
        ${_vtk_add_module_depends_link}
      PRIVATE
        ${_vtk_add_module_private_depends_link})

    set(_vtk_add_module_private_depends_forward_link)
    foreach (_vtk_add_module_private_depend IN LISTS _vtk_add_module_depends_link _vtk_add_module_private_depends)
      _vtk_module_get_module_property("${_vtk_add_module_private_depend}"
        PROPERTY "forward_link"
        VARIABLE  _vtk_add_module_forward_link)
      list(APPEND _vtk_add_module_private_depends_forward_link
        ${_vtk_add_module_forward_link})
    endforeach ()

    get_property(_vtk_add_module_optional_depends GLOBAL
      PROPERTY  "_vtk_module_${_vtk_build_module}_optional_depends")
    foreach (_vtk_add_module_optional_depend IN LISTS _vtk_add_module_optional_depends)
      _vtk_module_optional_dependency_exists("${_vtk_add_module_optional_depend}"
        SATISFIED_VAR _vtk_add_module_optional_depend_exists)
      if (_vtk_add_module_optional_depend_exists)
        set(_vtk_add_module_optional_depend_link "${_vtk_add_module_optional_depend}")
        if (_vtk_add_module_build_with_kit)
          get_property(_vtk_add_module_optional_depend_kit GLOBAL
            PROPERTY "_vtk_module_${_vtk_add_module_optional_depend}_kit")
          if (_vtk_add_module_optional_depend_kit STREQUAL _vtk_add_module_build_with_kit)
            # We're in the same kit; depend on the `-objects` library of the
            # module to avoid circular dependency (see explanation earlier)
            get_property(_vtk_add_module_optional_depend_target_name GLOBAL
              PROPERTY "_vtk_module_${_vtk_add_module_optional_depend}_target_name")
            set(_vtk_add_module_optional_depend_link "${_vtk_add_module_optional_depend_target_name}-objects")
          endif ()
        endif ()
        _vtk_module_get_module_property("${_vtk_add_module_optional_depend_link}"
          PROPERTY "forward_link"
          VARIABLE  _vtk_add_module_forward_link)
        list(APPEND _vtk_add_module_private_depends_forward_link
          ${_vtk_add_module_forward_link})
        target_link_libraries("${_vtk_add_module_real_target}"
          PRIVATE
            "${_vtk_add_module_optional_depend_link}")
      endif ()
      string(REPLACE "::" "_" _vtk_add_module_optional_depend_safe "${_vtk_add_module_optional_depend}")
      target_compile_definitions("${_vtk_add_module_real_target}"
        PRIVATE
          "VTK_MODULE_ENABLE_${_vtk_add_module_optional_depend_safe}=$<BOOL:${_vtk_add_module_optional_depend_exists}>")
    endforeach ()

    if (_vtk_add_module_private_depends_forward_link)
      list(REMOVE_DUPLICATES _vtk_add_module_private_depends_forward_link)
      _vtk_module_set_module_property("${_vtk_build_module}" APPEND
        PROPERTY  "forward_link"
        VALUE     "${_vtk_add_module_private_depends_forward_link}")
      target_link_libraries("${_vtk_add_module_real_target}"
        PUBLIC
          "${_vtk_add_module_private_depends_forward_link}")
    endif ()
  endif ()
  # Not needed for CMake 3.23+ as FILE_SET installation handles this
  # automatically.
  if (NOT _vtk_build_USE_FILE_SETS OR
      CMAKE_VERSION VERSION_LESS "3.23")
    _vtk_module_standard_includes(
      TARGET  "${_vtk_add_module_real_target}"
      ${_vtk_add_module_includes_interface}
      HEADERS_DESTINATION "${_vtk_build_HEADERS_DESTINATION}")
  endif ()

  vtk_module_autoinit(
    MODULES ${_vtk_add_module_depends}
            ${_vtk_add_module_private_depends}
            "${_vtk_build_module}"
    TARGETS "${_vtk_add_module_real_target}")

  set(_vtk_add_module_headers_build)
  set(_vtk_add_module_headers_install)
  # TODO: Perform this in `vtk_module_install_headers` so that manually
  # installed headers may participate in wrapping as well.
  foreach (_vtk_add_module_header IN LISTS _vtk_add_module_HEADERS)
    if (IS_ABSOLUTE "${_vtk_add_module_header}")
      list(APPEND _vtk_add_module_headers_build
        "${_vtk_add_module_header}")
    else ()
      list(APPEND _vtk_add_module_headers_build
        "${CMAKE_CURRENT_SOURCE_DIR}/${_vtk_add_module_header}")
    endif ()

    get_filename_component(_vtk_add_module_header_name "${_vtk_add_module_header}" NAME)
    list(APPEND _vtk_add_module_headers_install
      "\${_vtk_module_import_prefix}/${_vtk_build_HEADERS_DESTINATION}/${_vtk_add_module_header_name}")
  endforeach ()

  set_property(TARGET "${_vtk_add_module_real_target}"
    PROPERTY
      "INTERFACE_vtk_module_headers" "${_vtk_add_module_headers_build}")
  if (_vtk_build_INSTALL_HEADERS AND NOT _vtk_add_module_NO_INSTALL)
    set_property(TARGET "${_vtk_add_module_real_target}"
      PROPERTY
        "INTERFACE_vtk_module_headers_install" "${_vtk_add_module_headers_install}")
  endif ()

  set_property(TARGET "${_vtk_add_module_real_target}"
    PROPERTY
      "INTERFACE_vtk_module_include_marshal" "${_vtk_add_module_include_marshal}")
  get_property(_vtk_add_module_exclude_wrap GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_module}_exclude_wrap")
  set_property(TARGET "${_vtk_add_module_real_target}"
    PROPERTY
      "INTERFACE_vtk_module_exclude_wrap" "${_vtk_add_module_exclude_wrap}")
  if (NOT _vtk_add_module_exclude_wrap AND _vtk_build_ENABLE_WRAPPING)
    _vtk_module_write_wrap_hierarchy()
  endif ()

  # Make sure this file is excluded from the header tests
  set(_vtk_add_module_module_content "
/* VTK-HeaderTest-Exclude: ${_vtk_add_module_library_name}Module.h */
")

  if (NOT _vtk_add_module_AUTOINIT_INCLUDE)
    get_property(_vtk_add_module_AUTOINIT_INCLUDE GLOBAL
      PROPERTY  "_vtk_module_autoinit_include")
  endif ()

  set(_vtk_add_module_autoinit_include_header)
  if (_vtk_add_module_AUTOINIT_INCLUDE)
    set(_vtk_add_module_autoinit_include_header
      "#include ${_vtk_add_module_AUTOINIT_INCLUDE}")
  endif ()

  set(_vtk_add_module_autoinit_depends_includes)
  foreach (_vtk_add_module_autoinit_dependency IN LISTS _vtk_add_module_depends)
    get_property(_vtk_add_module_autoinit_dependency_target_name GLOBAL
      PROPERTY "_vtk_module_${_vtk_add_module_autoinit_dependency}_target_name")
    if (_vtk_add_module_autoinit_dependency_target_name)
      get_property(_vtk_add_module_depends_needs_autoinit
        TARGET    "${_vtk_add_module_autoinit_dependency_target_name}"
        PROPERTY  "INTERFACE_vtk_module_needs_autoinit")
    else ()
      set(_vtk_add_module_autoinit_dependency_target_name
        "${_vtk_add_module_autoinit_dependency}")
      get_property(_vtk_add_module_depends_needs_autoinit
        TARGET    "${_vtk_add_module_autoinit_dependency}"
        PROPERTY  "INTERFACE_vtk_module_needs_autoinit")
    endif ()
    if (NOT _vtk_add_module_depends_needs_autoinit)
      continue ()
    endif ()
    get_property(_vtk_add_module_depends_library_name
      TARGET    "${_vtk_add_module_autoinit_dependency_target_name}"
      PROPERTY  "INTERFACE_vtk_module_library_name")

    string(APPEND _vtk_add_module_autoinit_depends_includes
      "#include \"${_vtk_add_module_depends_library_name}Module.h\"\n")
  endforeach ()

  set(_vtk_add_module_autoinit_content)
  if (_vtk_add_module_autoinit_depends_includes)
    string(APPEND _vtk_add_module_autoinit_content
      "/* AutoInit dependencies. */\n${_vtk_add_module_autoinit_depends_includes}\n")
  endif ()

  get_property(_vtk_add_module_implementable GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_module}_implementable")
  get_property(_vtk_add_module_implements GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_module}_implements")
  if (_vtk_add_module_implementable)
    set_property(TARGET "${_vtk_add_module_real_target}"
      PROPERTY
        "INTERFACE_vtk_module_implementable" 1)
  endif ()

  # TODO: Make this a proper argument to be parsed.
  if (NOT _vtk_module_no_namespace_abi_mangling)
    # Include the ABI Namespace macros header
    string(APPEND _vtk_add_module_module_content
      "
/* Include ABI Namespace */
#include \"vtkABINamespace.h\"
")
  endif ()

  if (_vtk_add_module_implementable OR _vtk_add_module_implements)
    set_property(TARGET "${_vtk_add_module_real_target}"
      PROPERTY
        "INTERFACE_vtk_module_implements" "${_vtk_add_module_implements}")
    set_property(TARGET "${_vtk_add_module_real_target}"
      PROPERTY
        "INTERFACE_vtk_module_needs_autoinit" 1)

    string(APPEND _vtk_add_module_autoinit_content
      "
/* AutoInit implementations. */
#ifdef ${_vtk_add_module_library_name}_AUTOINIT_INCLUDE
#include ${_vtk_add_module_library_name}_AUTOINIT_INCLUDE
#endif
#ifdef ${_vtk_add_module_library_name}_AUTOINIT
${_vtk_add_module_autoinit_include_header}
VTK_MODULE_AUTOINIT(${_vtk_add_module_library_name})
#endif
")

    string(APPEND _vtk_add_module_module_content
      "${_vtk_add_module_autoinit_content}")
  endif ()

  if (NOT _vtk_add_module_HEADER_ONLY AND NOT _vtk_add_module_third_party)
    generate_export_header("${_vtk_add_module_real_target}"
      EXPORT_MACRO_NAME         "${_vtk_add_module_EXPORT_MACRO_PREFIX}_EXPORT"
      NO_EXPORT_MACRO_NAME      "${_vtk_add_module_EXPORT_MACRO_PREFIX}_NO_EXPORT"
      DEPRECATED_MACRO_NAME     "${_vtk_add_module_EXPORT_MACRO_PREFIX}_DEPRECATED"
      NO_DEPRECATED_MACRO_NAME  "${_vtk_add_module_EXPORT_MACRO_PREFIX}_NO_DEPRECATED"
      STATIC_DEFINE             "${_vtk_add_module_EXPORT_MACRO_PREFIX}_STATIC_DEFINE"
      EXPORT_FILE_NAME          "${_vtk_add_module_module_header_name}"
      CUSTOM_CONTENT_FROM_VARIABLE _vtk_add_module_module_content)
  endif ()

  _vtk_module_apply_properties("${_vtk_add_module_target_name}")
  _vtk_module_add_header_tests()

  if (NOT _vtk_add_module_NO_INSTALL)
    _vtk_module_install("${_vtk_add_module_target_name}")
    if (_vtk_add_module_build_with_kit)
      _vtk_module_install("${_vtk_add_module_target_name}-objects")
    endif ()
  endif ()

  get_property(_vtk_add_module_LICENSE_FILES GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_license_files")
  if (_vtk_add_module_LICENSE_FILES)
    if (_vtk_build_TARGET_SPECIFIC_COMPONENTS)
      string(PREPEND _vtk_build_LICENSE_COMPONENT "${_vtk_build_module}-")
    endif ()
    if (NOT _vtk_add_module_NO_INSTALL)
      install(
        FILES       ${_vtk_add_module_LICENSE_FILES}
        DESTINATION "${_vtk_build_LICENSE_DESTINATION}/${_vtk_add_module_library_name}/"
        COMPONENT   "${_vtk_build_LICENSE_COMPONENT}")
    endif ()
  endif ()

  if (_vtk_build_GENERATE_SPDX AND NOT _vtk_add_module_third_party)
    _vtk_module_generate_spdx(
      MODULE_NAME "${_vtk_add_module_library_name}"
      TARGET "${_vtk_add_module_target_name}-spdx"
      OUTPUT "${_vtk_add_module_library_name}.spdx"
      SKIP_REGEX "${_vtk_add_module_SPDX_SKIP_REGEX}"
      INPUT_FILES
        ${_vtk_add_module_SOURCES}
        ${_vtk_add_module_TEMPLATES}
        ${_vtk_add_module_PRIVATE_TEMPLATES}
        ${_vtk_add_module_HEADERS}
        ${_vtk_add_module_NOWRAP_HEADERS}
        ${_vtk_add_module_PRIVATE_HEADERS})
     add_dependencies("${_vtk_add_module_real_target}" "${_vtk_add_module_target_name}-spdx")

    if (_vtk_build_TARGET_SPECIFIC_COMPONENTS)
      string(PREPEND _vtk_build_SPDX_COMPONENT "${_vtk_build_module}-")
    endif ()
    if (NOT _vtk_add_module_NO_INSTALL)
      install(
        FILES       "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_add_module_library_name}.spdx"
        DESTINATION "${_vtk_build_SPDX_DESTINATION}"
        COMPONENT   "${_vtk_build_SPDX_COMPONENT}")
    endif ()
  endif ()
  if (_vtk_build_ENABLE_SERIALIZATION AND _vtk_add_module_include_marshal)
    vtk_module_serdes(
      MODULE             ${_vtk_build_module}
      EXPORT_MACRO_NAME  "${_vtk_add_module_EXPORT_MACRO_PREFIX}_EXPORT"
      EXPORT_FILE_NAME   "${_vtk_add_module_module_header_name}"
      REGISTRAR_HEADER   "${_vtk_add_module_serdes_registrar_header}"
      REGISTRAR_SOURCE   "${_vtk_add_module_serdes_registrar_source}"
      SERDES_SOURCES     _vtk_add_module_serdes_sources)
    vtk_module_sources(${_vtk_build_module}
      PRIVATE "${_vtk_add_module_serdes_sources}")
    # FIXME: The sources are generated, however their build rules are not.
    # _vtk_module_add_file_set("${_vtk_add_module_real_target}"
    #   NAME  vtk_module_serdes_sources
    #   FILES ${_vtk_add_module_serdes_sources})
  endif ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_add_header_tests

  Add header tests for a module. |module-impl|

  .. todo::

    Move this function out to be VTK-specific, probably into
    `vtkModuleTesting.cmake`. Each module would then need to manually call this
    function. It currently assumes it is in VTK itself.

  .. code-block:: cmake

    _vtk_module_add_header_tests()
#]==]
function (_vtk_module_add_header_tests)
  if (NOT BUILD_TESTING)
    return ()
  endif ()

  get_property(_vtk_add_header_tests_is_third_party GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_module}_third_party")
  if (_vtk_add_header_tests_is_third_party)
    return ()
  endif ()

  # TODO: Add test compiles which include each header file to ensure that
  # public headers have their includes satisfied by a public dependency.

  # Bad...
  if (NOT Python3_EXECUTABLE)
    return ()
  endif ()

  # Worse...
  if (NOT VTK_SOURCE_DIR)
    return ()
  endif ()

  if (NOT _vtk_add_module_EXCLUDE_HEADER_TEST)
    add_test(
      NAME    "${_vtk_build_module}-HeaderTest"
      COMMAND "${Python3_EXECUTABLE}"
              # TODO: What to do when using this from a VTK install?
              "${VTK_SOURCE_DIR}/Testing/Core/HeaderTesting.py"
              "${CMAKE_CURRENT_SOURCE_DIR}"
              "--export-macro"
              "${_vtk_add_module_EXPORT_MACRO}"
              "--headers"
              "${_vtk_add_module_HEADERS}"
              "${_vtk_add_module_NOWRAP_HEADERS}"
              "${_vtk_add_module_TEMPLATES}")
  endif ()
endfunction ()

#[==[
.. cmake:command:: vtk_module_install_headers

  Install headers. |module|

  Installing headers is done for normal modules by the :cmake:command:`vtk_module_add_module`
  function already. However, sometimes header structures are more complicated and
  need to be installed manually. This is common for third party modules or
  projects which use more than a single directory of headers for a module.

  To facilitate the installation of headers in various ways, the this function is
  available. This function honors the ``INSTALL_HEADERS``, ``HEADERS_DESTINATION``,
  ``HEADERS_COMPONENT``, and ``HEADERS_EXCLUDE_FROM_ALL`` arguments to :cmake:command:`vtk_module_build`.

  .. code-block:: cmake

    vtk_module_install_headers(
      [USE_RELATIVE_PATHS]
      [DIRECTORIES  <directory>...]
      [FILES        <file>...]
      [SUBDIR       <subdir>])

  Installation of header directories follows CMake's ``install`` function semantics
  with respect to trailing slashes.

  If ``USE_RELATIVE_PATHS`` is given, the directory part of any listed files will
  be added to the destination. Absolute paths will be computed relative to
  ``${CMAKE_CURRENT_BINARY_DIR}``.
#]==]
function (vtk_module_install_headers)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_install_headers
    "USE_RELATIVE_PATHS"
    "SUBDIR"
    "FILES;DIRECTORIES")

  if (_vtk_install_headers_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_install_headers: "
      "${_vtk_install_headers_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT _vtk_build_INSTALL_HEADERS)
    return ()
  endif ()

  if (NOT _vtk_install_headers_FILES AND NOT _vtk_install_headers_DIRECTORIES)
    return ()
  endif ()

  set(_vtk_install_headers_destination
    "${_vtk_build_HEADERS_DESTINATION}/${_vtk_install_headers_SUBDIR}")
  set(_vtk_install_headers_headers_component "${_vtk_build_HEADERS_COMPONENT}")
  if (_vtk_build_TARGET_SPECIFIC_COMPONENTS)
    string(PREPEND _vtk_install_headers_headers_component "${_vtk_build_module}-")
    if (_vtk_build_BUILD_WITH_KITS)
      get_property(_vtk_install_headers_build_with_kit GLOBAL
        PROPERTY "_vtk_module_${_vtk_build_module}_kit")
      if (_vtk_install_headers_build_with_kit)
        set(_vtk_install_headers_headers_component "${_vtk_install_headers_build_with_kit}-${_vtk_build_HEADERS_COMPONENT}")
      endif ()
    endif ()
  endif ()

  set(_vtk_install_headers_exclude_from_all "")
  if (_vtk_build_HEADERS_EXCLUDE_FROM_ALL)
    set(_vtk_install_headers_exclude_from_all "EXCLUDE_FROM_ALL")
  endif ()

  if (_vtk_install_headers_USE_RELATIVE_PATHS)
    set(_vtk_install_headers_destination_file_subdir "")
    foreach (_vtk_install_headers_file IN LISTS _vtk_install_headers_FILES)
      if (IS_ABSOLUTE "${_vtk_install_headers_file}")
        file(RELATIVE_PATH _vtk_install_headers_destination_file_from_binary_dir
          "${CMAKE_CURRENT_BINARY_DIR}"
          "${_vtk_install_headers_file}")
        get_filename_component(_vtk_install_headers_destination_file_subdir "${_vtk_install_headers_destination_file_from_binary_dir}" DIRECTORY)
      else ()
        get_filename_component(_vtk_install_headers_destination_file_subdir "${_vtk_install_headers_file}" DIRECTORY)
      endif ()
      install(
        FILES       ${_vtk_install_headers_file}
        DESTINATION "${_vtk_install_headers_destination}/${_vtk_install_headers_destination_file_subdir}"
        COMPONENT   "${_vtk_install_headers_headers_component}"
        ${_vtk_install_headers_exclude_from_all})
    endforeach ()
  elseif (_vtk_install_headers_FILES)
    install(
      FILES       ${_vtk_install_headers_FILES}
      DESTINATION "${_vtk_install_headers_destination}"
      COMPONENT   "${_vtk_install_headers_headers_component}"
      ${_vtk_install_headers_exclude_from_all})
  endif ()
  set(_vtk_install_headers_destination_directory_subdir "")
  foreach (_vtk_install_headers_directory IN LISTS _vtk_install_headers_DIRECTORIES)
    if (_vtk_install_headers_USE_RELATIVE_PATHS)
      if (IS_ABSOLUTE "${_vtk_install_headers_directory}")
        file(RELATIVE_PATH _vtk_install_headers_destination_directory_from_binary_dir
          "${CMAKE_CURRENT_BINARY_DIR}"
          "${_vtk_install_headers_directory}")
        get_filename_component(_vtk_install_headers_destination_directory_subdir "${_vtk_install_headers_destination_directory_from_binary_dir}" DIRECTORY)
      else ()
        get_filename_component(_vtk_install_headers_destination_directory_subdir "${_vtk_install_headers_directory}" DIRECTORY)
      endif ()
    endif ()
    install(
      DIRECTORY   "${_vtk_install_headers_directory}"
      DESTINATION "${_vtk_install_headers_destination}/${_vtk_install_headers_destination_directory_subdir}"
      COMPONENT   "${_vtk_install_headers_headers_component}"
      ${_vtk_install_headers_exclude_from_all})
  endforeach ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_apply_properties

  Apply properties to a module. |module-internal|

  Apply build properties to a target. Generally only useful to wrapping code or
  other modules that cannot use :cmake:command:`vtk_module_add_module` for some reason.

  .. code-block:: cmake

    _vtk_module_apply_properties(<target>
      [BASENAME <basename>])

  If ``BASENAME`` is given, it will be used instead of the target name as the basis
  for ``OUTPUT_NAME``. Full modules (as opposed to third party or other non-module
  libraries) always use the module's ``LIBRARY_NAME`` setting.

  The following target properties are set based on the arguments to the calling
  :cmake:command:`vtk_module_build call`

  - ``OUTPUT_NAME`` (based on the module's ``LIBRARY_NAME`` and
    ``vtk_module_build(LIBRARY_NAME_SUFFIX)``)
  - ``VERSION`` (based on ``vtk_module_build(VERSION)``)
  - ``SOVERSION`` (based on ``vtk_module_build(SOVERSION)``)
  - ``DEBUG_POSTFIX`` (on Windows unless already set via ``CMAKE_DEBUG_POSTFIX``)
#]==]
function (_vtk_module_apply_properties target)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_apply_properties
    ""
    "BASENAME"
    "")

  if (_vtk_apply_properties_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for _vtk_module_apply_properties: "
      "${_vtk_apply_properties_UNPARSED_ARGUMENTS}.")
  endif ()

  if (NOT DEFINED _vtk_apply_properties_BASENAME)
    set(_vtk_apply_properties_BASENAME "${target}")
  endif ()

  get_property(_vtk_add_module_type
    TARGET    "${target}"
    PROPERTY  TYPE)
  if (_vtk_add_module_type STREQUAL "OBJECT_LIBRARY" OR
      _vtk_add_module_type STREQUAL "INTERFACE_LIBRARY")
    return ()
  endif ()

  set(_vtk_add_module_library_name "${_vtk_apply_properties_BASENAME}")
  get_property(_vtk_add_module_target_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_target_name")
  if (_vtk_add_module_target_name STREQUAL "${target}")
    get_property(_vtk_add_module_library_name GLOBAL
      PROPERTY "_vtk_module_${_vtk_build_module}_library_name")
  endif ()
  set(_vtk_add_module_output_name "${_vtk_add_module_library_name}${_vtk_add_module_LIBRARY_NAME_SUFFIX}")
  if (_vtk_build_LIBRARY_NAME_SUFFIX)
    string(APPEND _vtk_add_module_output_name "-${_vtk_build_LIBRARY_NAME_SUFFIX}")
  endif ()

  set_target_properties("${target}"
    PROPERTIES
      OUTPUT_NAME "${_vtk_add_module_output_name}")

  if (_vtk_build_VERSION AND NOT _vtk_add_module_type STREQUAL "EXECUTABLE")
    set_target_properties("${target}"
      PROPERTIES
        VERSION "${_vtk_build_VERSION}")
  endif ()

  if (_vtk_build_SOVERSION)
    set_target_properties("${target}"
      PROPERTIES
        SOVERSION "${_vtk_build_SOVERSION}")
  endif ()

  if (WIN32 AND NOT DEFINED CMAKE_DEBUG_POSTFIX)
    set_target_properties("${target}"
      PROPERTIES
        DEBUG_POSTFIX "d")
  endif ()

  # rpath settings
  if (NOT _vtk_add_module_type STREQUAL "EXECUTABLE")
    set_property(TARGET "${target}"
      PROPERTY
        BUILD_RPATH_USE_ORIGIN 1)
    if (UNIX)
      if (APPLE)
        set(_vtk_build_origin_rpath_prefix
          "@loader_path")
      else ()
        set(_vtk_build_origin_rpath_prefix
          "$ORIGIN")
      endif ()

      set_property(TARGET "${target}" APPEND
        PROPERTY
          INSTALL_RPATH "${_vtk_build_origin_rpath_prefix}")
    endif ()
  endif ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_install

  Install a module target. |module-internal|

  Install a target within the module context. Generally only useful to wrapping
  code, modules that cannot use :cmake:command:`vtk_module_add_module` for some reason, or
  modules which create utility targets that need installed.

  .. code-block:: cmake

    _vtk_module_install(<target>)

  This function uses the various installation options to :cmake:command:`vtk_module_build`
  function to keep the install uniform.
#]==]
function (_vtk_module_install target)
  set(_vtk_install_export)
  if (_vtk_build_INSTALL_EXPORT)
    list(APPEND _vtk_install_export
      EXPORT "${_vtk_build_INSTALL_EXPORT}")
  endif ()

  set(_vtk_install_headers_component "${_vtk_build_HEADERS_COMPONENT}")
  set(_vtk_install_targets_component "${_vtk_build_TARGETS_COMPONENT}")
  if (_vtk_build_TARGET_SPECIFIC_COMPONENTS)
    if (_vtk_build_kit)
      string(PREPEND _vtk_install_headers_component "${_vtk_build_kit}-")
      string(PREPEND _vtk_install_targets_component "${_vtk_build_kit}-")
    else ()
      string(PREPEND _vtk_install_headers_component "${_vtk_build_module}-")
      string(PREPEND _vtk_install_targets_component "${_vtk_build_module}-")
      if (_vtk_build_BUILD_WITH_KITS)
        get_property(_vtk_install_kit GLOBAL
          PROPERTY "_vtk_module_${_vtk_build_module}_kit")
        if (_vtk_install_kit)
          set(_vtk_install_headers_component "${_vtk_install_kit}-${_vtk_build_HEADERS_COMPONENT}")
          set(_vtk_install_targets_component "${_vtk_install_kit}-${_vtk_build_TARGETS_COMPONENT}")
        endif ()
      endif ()
    endif ()
  endif ()

  set(_vtk_install_file_set_args)
  # XXX(cmake-3.23): file sets
  if (_vtk_build_USE_FILE_SETS AND
      CMAKE_VERSION VERSION_GREATER_EQUAL "3.23")
    set(_vtk_install_file_sets_destination "${_vtk_build_HEADERS_DESTINATION}")
    if (_vtk_add_module_HEADERS_SUBDIR)
      string(APPEND _vtk_install_file_sets_destination
        "/${_vtk_add_module_HEADERS_SUBDIR}")
    endif ()

    get_property(_vtk_install_available_file_sets
      TARGET    "${target}"
      PROPERTY  INTERFACE_HEADER_SETS)
    foreach (_vtk_install_file_set IN LISTS _vtk_install_available_file_sets)
      list(APPEND _vtk_install_file_set_args
        FILE_SET "${_vtk_install_file_set}"
          DESTINATION "${_vtk_install_file_sets_destination}"
          COMPONENT   "${_vtk_install_headers_component}")
    endforeach ()
  endif ()

  install(
    TARGETS             "${target}"
    ${_vtk_install_export}
    ${ARGN}
    ARCHIVE
      DESTINATION "${_vtk_build_ARCHIVE_DESTINATION}"
      COMPONENT   "${_vtk_install_targets_component}"
    LIBRARY
      DESTINATION "${_vtk_build_LIBRARY_DESTINATION}"
      COMPONENT   "${_vtk_install_targets_component}"
      NAMELINK_COMPONENT "${_vtk_build_HEADERS_COMPONENT}"
    RUNTIME
      DESTINATION "${_vtk_build_RUNTIME_DESTINATION}"
      COMPONENT   "${_vtk_install_targets_component}"
    ${_vtk_install_file_set_args})
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_add_executable

  Create a module executable. |module|

  Some modules may have associated executables with them. By using this function,
  the target will be installed following the options given to the associated
  :cmake:command:`vtk_module_build` command. Its name will also be changed according to the
  ``LIBRARY_NAME_SUFFIX`` option.

  .. code-block:: cmake

    vtk_module_add_executable(<name>
      [NO_INSTALL]
      [DEVELOPMENT]
      [BASENAME <basename>]
      <source>...)

  If ``NO_INSTALL`` is specified, the executable will not be installed. If
  ``BASENAME`` is given, it will be used as the name of the executable rather than
  the target name.

  If ``DEVELOPMENT`` is given, it marks the executable as a development tool and
  will not be installed if ``INSTALL_HEADERS`` is not set for the associated
  :cmake:command:`vtk_module_build` command.

  If the executable being built is the module, its module properties are used
  rather than ``BASENAME``. In addition, the dependencies of the module will be
  linked.
#]==]
function (vtk_module_add_executable name)
  cmake_parse_arguments(PARSE_ARGV 1 _vtk_add_executable
    "NO_INSTALL;DEVELOPMENT"
    "BASENAME"
    "")

  if (NOT _vtk_add_executable_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "The ${name} executable must have at least one source file.")
  endif ()

  if (_vtk_add_executable_NO_INSTALL AND _vtk_add_executable_DEVELOPMENT)
    message(FATAL_ERROR
      "Both `NO_INSTALL` and `DEVELOPMENT` may not be specified.")
  endif ()

  set(_vtk_add_executable_target_name "${name}")
  set(_vtk_add_executable_library_name "${name}")
  if (name STREQUAL _vtk_build_module)
    if (_vtk_add_executable_NO_INSTALL)
      message(FATAL_ERROR
        "The executable ${_vtk_build_module} module may not use `NO_INSTALL`.")
    endif ()
    if (DEFINED _vtk_add_executable_BASENAME)
      message(FATAL_ERROR
        "The executable ${_vtk_build_module} module may not pass `BASENAME` "
        "when adding the executable; it is controlled via `LIBRARY_NAME` in "
        "the associated `vtk.module` file.")
    endif ()
    get_property(_vtk_add_executable_target_name GLOBAL
      PROPERTY "_vtk_module_${_vtk_build_module}_target_name")
    get_property(_vtk_add_executable_library_name GLOBAL
      PROPERTY "_vtk_module_${_vtk_build_module}_library_name")
  endif ()

  if (_vtk_add_executable_DEVELOPMENT AND NOT _vtk_build_INSTALL_HEADERS)
    set(_vtk_add_executable_NO_INSTALL ON)
  endif ()

  # Set up rpaths
  set(CMAKE_BUILD_RPATH_USE_ORIGIN 1)
  if (UNIX)
    file(RELATIVE_PATH _vtk_add_executable_relpath
      "/prefix/${_vtk_build_RUNTIME_DESTINATION}"
      "/prefix/${_vtk_build_LIBRARY_DESTINATION}")
    if (APPLE)
      set(_vtk_add_executable_origin_rpath_prefix
        "@executable_path")
    else ()
      set(_vtk_add_executable_origin_rpath_prefix
        "$ORIGIN")
    endif ()

    list(APPEND CMAKE_INSTALL_RPATH
      "${_vtk_add_executable_origin_rpath_prefix}/${_vtk_add_executable_relpath}")
  endif ()

  add_executable("${_vtk_add_executable_target_name}")
  target_sources("${_vtk_add_executable_target_name}"
    PRIVATE
      ${_vtk_add_executable_UNPARSED_ARGUMENTS})

  if (name STREQUAL _vtk_build_module AND NOT _vtk_add_executable_target_name STREQUAL _vtk_build_module)
    add_executable("${_vtk_build_module}" ALIAS
      "${_vtk_add_executable_target_name}")
  endif ()

  if (name STREQUAL _vtk_build_module)
    get_property(_vtk_real_target_kit GLOBAL
      PROPERTY "_vtk_module_${_vtk_build_module}_kit")
    if (_vtk_real_target_kit)
      message(FATAL_ERROR
        "Executable module ${_vtk_build_module} is declared to be part of a "
        "kit; this is not possible.")
    endif ()

    get_property(_vtk_add_executable_depends GLOBAL
      PROPERTY  "_vtk_module_${_vtk_build_module}_depends")
    get_property(_vtk_add_executable_private_depends GLOBAL
      PROPERTY  "_vtk_module_${_vtk_build_module}_private_depends")
    target_link_libraries("${_vtk_add_executable_target_name}"
      PUBLIC
        ${_vtk_add_executable_depends}
      PRIVATE
        ${_vtk_add_executable_private_depends})
    get_property(_vtk_add_executable_optional_depends GLOBAL
      PROPERTY  "_vtk_module_${_vtk_build_module}_optional_depends")
    foreach (_vtk_add_executable_optional_depend IN LISTS _vtk_add_executable_optional_depends)
      _vtk_module_optional_dependency_exists("${_vtk_add_executable_optional_depend}"
        SATISFIED_VAR _vtk_add_executable_optional_depend_exists)
      string(REPLACE "::" "_" _vtk_add_executable_optional_depend_safe "${_vtk_add_executable_optional_depend}")
      target_compile_definitions("${_vtk_add_executable_target_name}"
        PRIVATE
          "VTK_MODULE_ENABLE_${_vtk_add_executable_optional_depend_safe}=$<BOOL:{_vtk_add_executable_optional_depend_exists}>")
    endforeach ()

    if (_vtk_module_warnings)
      if (_vtk_add_executable_depends)
        message(WARNING
          "Executable module ${_vtk_build_module} has public dependencies; this "
          "shouldn't be necessary.")
      endif ()
    endif ()
  endif ()

  if (_vtk_build_UTILITY_TARGET)
    target_link_libraries("${_vtk_add_executable_target_name}"
      PRIVATE
        "${_vtk_build_UTILITY_TARGET}")
  endif ()

  set(_vtk_add_executable_property_args)
  if (DEFINED _vtk_add_executable_BASENAME)
    list(APPEND _vtk_add_executable_property_args
      BASENAME "${_vtk_add_executable_BASENAME}")
  endif ()

  _vtk_module_apply_properties("${_vtk_add_executable_target_name}"
    ${_vtk_add_executable_property_args})
  _vtk_module_standard_includes(TARGET "${_vtk_add_executable_target_name}")

  if (NOT _vtk_add_executable_NO_INSTALL)
    _vtk_module_install("${_vtk_add_executable_target_name}")
  endif ()
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_find_package

  Find a package. |module|

  A wrapper around ``find_package`` that records information for use so that the
  same targets may be found when finding this package.

  Modules may need to find external dependencies. CMake often provides modules to
  find these dependencies, but when imported targets are involved, these.need to
  also be found from dependencies of the current project. Since the benefits of
  imported targets greatly outweighs not using them, it is preferred to use them.

  The module system provides the :cmake:command:`vtk_module_find_package` function in order
  to extend ``find_package`` support to include finding the dependencies from an
  install of the project.

  .. code-block:: cmake

    vtk_module_find_package(
      [PRIVATE] [PRIVATE_IF_SHARED] [CONFIG_MODE]
      PACKAGE               <package>
      [VERSION              <version>]
      [COMPONENTS           <component>...]
      [OPTIONAL_COMPONENTS  <component>...]
      [FORWARD_VERSION_REQ  <MAJOR|MINOR|PATCH|EXACT>]
      [VERSION_VAR          <variable>])

  * ``PACKAGE``: The name of the package to find.
  * ``VERSION``: The minimum version of the package that is required.
  * ``COMPONENTS``: Components of the package which are required.
  * ``OPTIONAL_COMPONENTS``: Components of the package which may be missing.
  * ``FORWARD_VERSION_REQ``: If provided, the found version will be promoted to
    the minimum version required matching the given version scheme.
  * ``VERSION_VAR``: The variable to use as the provided version (defaults to
    ``<PACKAGE>_VERSION``). It may contain ``@`` in which case it will be
    configured. This is useful for modules which only provide components of the
    actual version number.
  * ``CONFIG_MODE``: If present, pass ``CONFIG`` to the underlying ``find_package``
    call.
  * ``PRIVATE``: The dependency should not be exported to the install.
  * ``PRIVATE_IF_SHARED``: The dependency should not be exported to the install
    if the module is built as a ``SHARED`` library.

  The ``PACKAGE`` argument is the only required argument. The rest are optional.

  Note that ``PRIVATE`` is *only* applicable for private dependencies on interface
  targets (basically, header libraries) because some platforms require private
  shared libraries dependencies to be present when linking dependent libraries
  and executables as well. Such usages should additionally be used only via a
  ``$<BUILD_INTERFACE>`` generator expression to avoid putting the target name into
  the install tree at all.
#]==]
macro (vtk_module_find_package)
  # This needs to be a macro because find modules typically set variables which
  # may need to be available in the calling scope. If we declare that it only
  # works with imported targets (which is the primary motivating factor behind
  # this function), we can instead make it a function at the cost of any
  # non-target variables a module might want to set being available. It is
  # unlikely that this will be the case for all callers.
  if (NOT _vtk_build_module)
    message(FATAL_ERROR
      "`vtk_module_find_package` may only be called when building a VTK "
      "module.")
  endif ()

  # Note: when adding arguments here, add them to the `unset` block at the end
  # of the function.
  cmake_parse_arguments(_vtk_find_package
    "PRIVATE;PRIVATE_IF_SHARED;CONFIG_MODE"
    "PACKAGE;VERSION;FORWARD_VERSION_REQ;VERSION_VAR"
    "COMPONENTS;OPTIONAL_COMPONENTS"
    ${ARGN})

  if (_vtk_find_package_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_find_package: "
      "${_vtk_find_package_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_find_package_PACKAGE)
    message(FATAL_ERROR
      "The `PACKAGE` argument is required.")
  endif ()

  if (_vtk_find_package_PRIVATE AND _vtk_find_package_PRIVATE_IF_SHARED)
    message(FATAL_ERROR
      "The `PRIVATE` and `PRIVATE_IF_SHARED` arguments are mutually "
      "exclusive.")
  endif ()

  if (DEFINED _vtk_find_package_FORWARD_VERSION_REQ)
    if (_vtk_find_package_PRIVATE)
      message(FATAL_ERROR
        "The `FORWARD_VERSION_REQ` argument is incompatible with the "
        "`PRIVATE` flag.")
    endif ()

    if (NOT _vtk_find_package_FORWARD_VERSION_REQ STREQUAL "MAJOR" AND
        NOT _vtk_find_package_FORWARD_VERSION_REQ STREQUAL "MINOR" AND
        NOT _vtk_find_package_FORWARD_VERSION_REQ STREQUAL "PATCH" AND
        NOT _vtk_find_package_FORWARD_VERSION_REQ STREQUAL "EXACT")
      message(FATAL_ERROR
        "The `FORWARD_VERSION_REQ` argument must be one of `MAJOR`, `MINOR`, "
        "`PATCH`, or `EXACT`.")
    endif ()
  endif ()

  if (NOT DEFINED _vtk_find_package_VERSION_VAR)
    set(_vtk_find_package_VERSION_VAR
      "${_vtk_find_package_PACKAGE}_VERSION")
  endif ()

  set(_vtk_find_package_config)
  if (_vtk_find_package_CONFIG_MODE)
    set(_vtk_find_package_config "CONFIG")
  endif ()

  find_package("${_vtk_find_package_PACKAGE}"
    ${_vtk_find_package_VERSION}
    ${_vtk_find_package_config}
    COMPONENTS          ${_vtk_find_package_COMPONENTS}
    OPTIONAL_COMPONENTS ${_vtk_find_package_OPTIONAL_COMPONENTS})
  if (NOT ${_vtk_find_package_PACKAGE}_FOUND)
    message(FATAL_ERROR
      "Could not find the ${_vtk_find_package_PACKAGE} external dependency.")
    return ()
  endif ()

  set(_vtk_find_package_optional_components_found)
  foreach (_vtk_find_package_optional_component IN LISTS _vtk_find_package_OPTIONAL_COMPONENTS)
    if (${_vtk_find_package_PACKAGE}_${_vtk_find_package_optional_component}_FOUND)
      list(APPEND _vtk_find_package_optional_components_found
        "${_vtk_find_package_optional_component}")
    endif ()
  endforeach ()

  set_property(GLOBAL APPEND
    PROPERTY
      "_vtk_module_find_packages_${_vtk_build_PACKAGE}" "${_vtk_find_package_PACKAGE}")
  set(_vtk_find_package_base "_vtk_module_find_package_${_vtk_build_module}")
  set_property(GLOBAL APPEND
    PROPERTY
      "${_vtk_find_package_base}" "${_vtk_find_package_PACKAGE}")
  set(_vtk_find_package_base_package "${_vtk_find_package_base}_${_vtk_find_package_PACKAGE}")
  set_property(GLOBAL
    PROPERTY
      "${_vtk_find_package_base_package}_private" "${_vtk_find_package_PRIVATE}")
  set_property(GLOBAL
    PROPERTY
      "${_vtk_find_package_base_package}_private_if_shared" "${_vtk_find_package_PRIVATE_IF_SHARED}")
  set_property(GLOBAL
    PROPERTY
      "${_vtk_find_package_base_package}_version" "${_vtk_find_package_VERSION}")
  set_property(GLOBAL
    PROPERTY
      "${_vtk_find_package_base_package}_config" "${_vtk_find_package_CONFIG_MODE}")
  set_property(GLOBAL APPEND
    PROPERTY
      "${_vtk_find_package_base_package}_components" "${_vtk_find_package_COMPONENTS}")
  set_property(GLOBAL APPEND
    PROPERTY
      "${_vtk_find_package_base_package}_optional_components" "${_vtk_find_package_OPTIONAL_COMPONENTS}")
  set_property(GLOBAL APPEND
    PROPERTY
      "${_vtk_find_package_base_package}_optional_components_found" "${_vtk_find_package_optional_components_found}")
  set_property(GLOBAL
    PROPERTY
      "${_vtk_find_package_base_package}_exact" "0")
  if (DEFINED _vtk_find_package_FORWARD_VERSION_REQ)
    string(FIND "${_vtk_find_package_VERSION_VAR}" "@" _vtk_find_package_idx)
    if (_vtk_find_package_idx EQUAL -1)
      if (NOT DEFINED "${_vtk_find_package_VERSION_VAR}")
        message(FATAL_ERROR
          "The `${_vtk_find_package_VERSION_VAR}` variable is not defined.")
      endif ()
      set(_vtk_find_package_version "${${_vtk_find_package_VERSION_VAR}}")
    else ()
      string(CONFIGURE "${_vtk_find_package_VERSION_VAR}" _vtk_find_package_version)
    endif ()
    unset(_vtk_find_package_idx)

    if ("${_vtk_find_package_version}" STREQUAL "")
      message(FATAL_ERROR
        "The `${_vtk_find_package_PACKAGE}` version is empty.")
    endif ()

    if (_vtk_find_package_FORWARD_VERSION_REQ STREQUAL "MAJOR")
      set(_vtk_find_package_version_regex "^\([^.]*\).*")
    elseif (_vtk_find_package_FORWARD_VERSION_REQ STREQUAL "MINOR")
      set(_vtk_find_package_version_regex "^\([^.]*.[^.]*\).*")
    elseif (_vtk_find_package_FORWARD_VERSION_REQ STREQUAL "PATCH")
      set(_vtk_find_package_version_regex "^\([^.]*.[^.]*.[^.]*\).*")
    elseif (_vtk_find_package_FORWARD_VERSION_REQ STREQUAL "EXACT")
      set(_vtk_find_package_version_regex "^\\(.*\\)$")
      set_property(GLOBAL
        PROPERTY
          "${_vtk_find_package_base_package}_exact" "1")
    endif ()

    string(REGEX REPLACE "${_vtk_find_package_version_regex}" "\\1"
      _vtk_find_package_found_version "${_vtk_find_package_version}")
    unset(_vtk_find_package_version_regex)
    unset(_vtk_find_package_version)

    set_property(GLOBAL
      PROPERTY
        "${_vtk_find_package_base_package}_version" "${_vtk_find_package_found_version}")
    unset(_vtk_find_package_found_version)
  endif ()

  unset(_vtk_find_package_base)
  unset(_vtk_find_package_base_package)
  unset(_vtk_find_package_COMPONENTS)
  unset(_vtk_find_package_FORWARD_VERSION_REQ)
  unset(_vtk_find_package_OPTIONAL_COMPONENTS)
  unset(_vtk_find_package_PACKAGE)
  unset(_vtk_find_package_PRIVATE)
  unset(_vtk_find_package_UNPARSED_ARGUMENTS)
  unset(_vtk_find_package_VERSION)
  unset(_vtk_find_package_VERSION_VAR)
endmacro ()

#[==[.rst:
.. cmake:command:: vtk_module_export_find_packages

  Export find_package calls for dependencies. |module|

  When installing a project that is meant to be found via ``find_package`` from
  CMake, using imported targets in the build means that imported targets need to
  be created during the ``find_package`` as well. This function writes a file
  suitable for inclusion from a ``<package>-config.cmake`` file to satisfy
  dependencies. It assumes that the exported targets are named
  ``${CMAKE_FIND_PACKAGE_NAME}::${component}``. Dependent packages will only be
  found if a requested component requires the package to be found either directly
  or transitively.

  .. code-block:: cmake

    vtk_module_export_find_packages(
      CMAKE_DESTINATION <directory>
      FILE_NAME         <filename>
      [COMPONENT        <component>]
      MODULES           <module>...)

  The file will be named according to the ``FILE_NAME`` argument will be installed
  into ``CMAKE_DESTINATION`` in the build and install trees with the given
  filename. If not provided, the ``development`` component will be used.

  The ``vtk_module_find_package`` calls made by the modules listed in ``MODULES``
  will be exported to this file.
#]==]
function (vtk_module_export_find_packages)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_export
    ""
    "CMAKE_DESTINATION;FILE_NAME;COMPONENT"
    "MODULES")

  if (_vtk_export_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_export_find_packages: "
      "${_vtk_export_UNPARSED_ARGUMENTS}")
  endif ()

  if (NOT DEFINED _vtk_export_CMAKE_DESTINATION)
    message(FATAL_ERROR
      "The `CMAKE_DESTINATION` is required.")
  endif ()

  if (NOT DEFINED _vtk_export_FILE_NAME)
    message(FATAL_ERROR
      "The `FILE_NAME` is required.")
  endif ()

  if (NOT DEFINED _vtk_export_COMPONENT)
    set(_vtk_export_COMPONENT "development")
  endif ()

  set(_vtk_export_prelude
"set(_vtk_module_find_package_quiet)
if (\${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
  set(_vtk_module_find_package_quiet QUIET)
endif ()

set(_vtk_module_find_package_components_checked)
set(_vtk_module_find_package_components_to_check
  \${\${CMAKE_FIND_PACKAGE_NAME}_FIND_COMPONENTS})
set(_vtk_module_find_package_components)
set(_vtk_module_find_package_components_required)
while (_vtk_module_find_package_components_to_check)
  list(GET _vtk_module_find_package_components_to_check 0 _vtk_module_component)
  list(REMOVE_AT _vtk_module_find_package_components_to_check 0)
  if (_vtk_module_component IN_LIST _vtk_module_find_package_components_checked)
    continue ()
  endif ()
  list(APPEND _vtk_module_find_package_components_checked
    \"\${_vtk_module_component}\")

  # Any 'components' with `::` are not from our package and must have been
  # provided/satisfied elsewhere.
  if (_vtk_module_find_package_components MATCHES \"::\")
    continue ()
  endif ()

  list(APPEND _vtk_module_find_package_components
    \"\${_vtk_module_component}\")
  if (\${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED_\${_vtk_module_component})
    list(APPEND _vtk_module_find_package_components_required
      \"\${_vtk_module_component}\")
  endif ()

  if (TARGET \"\${CMAKE_FIND_PACKAGE_NAME}::\${_vtk_module_component}\")
    set(_vtk_module_find_package_component_target \"\${CMAKE_FIND_PACKAGE_NAME}::\${_vtk_module_component}\")
  elseif (TARGET \"\${_vtk_module_component}\")
    set(_vtk_module_find_package_component_target \"\${_vtk_module_component}\")
  else ()
    # No such target for the component; skip.
    continue ()
  endif ()
  get_property(_vtk_module_find_package_depends
    TARGET    \"\${_vtk_module_find_package_component_target}\"
    PROPERTY  \"INTERFACE_vtk_module_depends\")
  string(REPLACE \"\${CMAKE_FIND_PACKAGE_NAME}::\" \"\" _vtk_module_find_package_depends \"\${_vtk_module_find_package_depends}\")
  list(APPEND _vtk_module_find_package_components_to_check
    \${_vtk_module_find_package_depends})
  get_property(_vtk_module_find_package_depends
    TARGET    \"\${_vtk_module_find_package_component_target}\"
    PROPERTY  \"INTERFACE_vtk_module_private_depends\")
  string(REPLACE \"\${CMAKE_FIND_PACKAGE_NAME}::\" \"\" _vtk_module_find_package_depends \"\${_vtk_module_find_package_depends}\")
  list(APPEND _vtk_module_find_package_components_to_check
    \${_vtk_module_find_package_depends})
  get_property(_vtk_module_find_package_depends
    TARGET    \"\${_vtk_module_find_package_component_target}\"
    PROPERTY  \"INTERFACE_vtk_module_optional_depends\")
  foreach (_vtk_module_find_package_depend IN LISTS _vtk_module_find_package_depends)
    if (TARGET \"\${_vtk_module_find_package_depend}\")
      string(REPLACE \"\${CMAKE_FIND_PACKAGE_NAME}::\" \"\" _vtk_module_find_package_depend \"\${_vtk_module_find_package_depend}\")
      list(APPEND _vtk_module_find_package_components_to_check
        \"\${_vtk_module_find_package_depend}\")
    endif ()
  endforeach ()
  get_property(_vtk_module_find_package_depends
    TARGET    \"\${_vtk_module_find_package_component_target}\"
    PROPERTY  \"INTERFACE_vtk_module_forward_link\")
  string(REPLACE \"\${CMAKE_FIND_PACKAGE_NAME}::\" \"\" _vtk_module_find_package_depends \"\${_vtk_module_find_package_depends}\")
  list(APPEND _vtk_module_find_package_components_to_check
    \${_vtk_module_find_package_depends})

  get_property(_vtk_module_find_package_kit
    TARGET    \"\${_vtk_module_find_package_component_target}\"
    PROPERTY  \"INTERFACE_vtk_module_kit\")
  if (_vtk_module_find_package_kit)
    get_property(_vtk_module_find_package_kit_modules
      TARGET    \"\${_vtk_module_find_package_kit}\"
      PROPERTY  \"INTERFACE_vtk_kit_kit_modules\")
    string(REPLACE \"\${CMAKE_FIND_PACKAGE_NAME}::\" \"\" _vtk_module_find_package_kit_modules \"\${_vtk_module_find_package_kit_modules}\")
    list(APPEND _vtk_module_find_package_components_to_check
      \${_vtk_module_find_package_kit_modules})
  endif ()
endwhile ()
unset(_vtk_module_find_package_component_target)
unset(_vtk_module_find_package_components_to_check)
unset(_vtk_module_find_package_components_checked)
unset(_vtk_module_component)
unset(_vtk_module_find_package_depend)
unset(_vtk_module_find_package_depends)
unset(_vtk_module_find_package_kit)
unset(_vtk_module_find_package_kit_modules)

if (_vtk_module_find_package_components)
  list(REMOVE_DUPLICATES _vtk_module_find_package_components)
endif ()
if (_vtk_module_find_package_components_required)
  list(REMOVE_DUPLICATES _vtk_module_find_package_components_required)
endif ()\n\n")

  set(_vtk_export_build_content)
  set(_vtk_export_install_content)
  foreach (_vtk_export_module IN LISTS _vtk_export_MODULES)
    get_property(_vtk_export_target_name GLOBAL
      PROPERTY "_vtk_module_${_vtk_export_module}_target_name")
    # Use the export name of the target if it has one set.
    get_property(_vtk_export_target_has_export_name
      TARGET    "${_vtk_export_target_name}"
      PROPERTY  EXPORT_NAME SET)
    if (_vtk_export_target_has_export_name)
      get_property(_vtk_export_target_name
        TARGET    "${_vtk_export_target_name}"
        PROPERTY  EXPORT_NAME)
    endif ()
    set(_vtk_export_base "_vtk_module_find_package_${_vtk_export_module}")
    get_property(_vtk_export_packages GLOBAL
      PROPERTY "${_vtk_export_base}")
    if (NOT _vtk_export_packages)
      continue ()
    endif ()

    set(_vtk_export_module_prelude
"set(_vtk_module_find_package_enabled OFF)
set(_vtk_module_find_package_is_required OFF)
set(_vtk_module_find_package_fail_if_not_found OFF)
if (_vtk_module_find_package_components)
  if (\"${_vtk_export_target_name}\" IN_LIST _vtk_module_find_package_components)
    set(_vtk_module_find_package_enabled ON)
    if (\"${_vtk_export_target_name}\" IN_LIST _vtk_module_find_package_components_required)
      set(_vtk_module_find_package_is_required \"\${\${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED}\")
      set(_vtk_module_find_package_fail_if_not_found ON)
    endif ()
  endif ()
else ()
  set(_vtk_module_find_package_enabled ON)
  set(_vtk_module_find_package_is_required \"\${\${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED}\")
  set(_vtk_module_find_package_fail_if_not_found ON)
endif ()

if (_vtk_module_find_package_enabled)
  set(_vtk_module_find_package_required)
  if (_vtk_module_find_package_is_required)
    set(_vtk_module_find_package_required REQUIRED)
  endif ()\n\n")

    list(REMOVE_DUPLICATES _vtk_export_packages)
    set(_vtk_export_module_build_content)
    set(_vtk_export_module_install_content)
    foreach (_vtk_export_package IN LISTS _vtk_export_packages)
      set(_vtk_export_base_package "${_vtk_export_base}_${_vtk_export_package}")
      get_property(_vtk_export_private GLOBAL
        PROPERTY "${_vtk_export_base_package}_private")
      get_property(_vtk_export_private_if_shared GLOBAL
        PROPERTY "${_vtk_export_base_package}_private_if_shared")
      if (_vtk_export_private_if_shared)
        get_property(_vtk_export_kit_name GLOBAL
          PROPERTY "_vtk_module_${_vtk_export_module}_kit")
        if (_vtk_export_kit_name AND TARGET "${_vtk_export_kit_name}")
          get_property(_vtk_export_kit_is_alias
            TARGET    "${_vtk_export_kit_name}"
            PROPERTY  ALIASED_TARGET
            SET)
          if (_vtk_export_kit_is_alias)
            get_property(_vtk_export_kit_name
              TARGET    "${_vtk_export_kit_name}"
              PROPERTY  ALIASED_TARGET)
          endif ()
          get_property(_vtk_export_module_type
            TARGET "${_vtk_export_kit_name}"
            PROPERTY  TYPE)
        else ()
          vtk_module_get_property("${_vtk_export_module}"
            PROPERTY  TYPE
            VARIABLE  _vtk_export_module_type)
        endif ()
        if (_vtk_export_module_type STREQUAL "SHARED_LIBRARY")
          set(_vtk_export_private 1)
        endif ()
      endif ()
      get_property(_vtk_export_version GLOBAL
        PROPERTY "${_vtk_export_base_package}_version")
      get_property(_vtk_export_config GLOBAL
        PROPERTY "${_vtk_export_base_package}_config")
      get_property(_vtk_export_exact GLOBAL
        PROPERTY "${_vtk_export_base_package}_exact")
      get_property(_vtk_export_components GLOBAL
        PROPERTY "${_vtk_export_base_package}_components")
      get_property(_vtk_export_optional_components GLOBAL
        PROPERTY "${_vtk_export_base_package}_optional_components")
      get_property(_vtk_export_optional_components_found GLOBAL
        PROPERTY "${_vtk_export_base_package}_optional_components_found")

      # Assume that any found optional components end up being required.
      if (${_vtk_export_base_package}_optional_components_found)
        list(REMOVE_ITEM _vtk_export_optional_components
          ${_vtk_export_optional_components_found})
        list(APPEND _vtk_export_components
          ${_vtk_export_optional_components_found})
      endif ()

      set(_vtk_export_config_arg)
      if (_vtk_export_config)
        set(_vtk_export_config_arg CONFIG)
      endif ()

      set(_vtk_export_exact_arg)
      if (_vtk_export_exact)
        set(_vtk_export_exact_arg EXACT)
      endif ()

      set(_vtk_export_module_content
"  find_package(${_vtk_export_package}
    ${_vtk_export_version}
    ${_vtk_export_exact_arg}
    ${_vtk_export_config_arg}
    \${_vtk_module_find_package_quiet}
    \${_vtk_module_find_package_required}
    COMPONENTS          ${_vtk_export_components}
    OPTIONAL_COMPONENTS ${_vtk_export_optional_components})
  if (NOT ${_vtk_export_package}_FOUND AND _vtk_module_find_package_fail_if_not_found)
    if (NOT \${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
      message(STATUS
        \"Could not find the \${CMAKE_FIND_PACKAGE_NAME} package due to a \"
        \"missing dependency: ${_vtk_export_package}\")
    endif ()
    set(\"\${CMAKE_FIND_PACKAGE_NAME}_${_vtk_export_target_name}_FOUND\" 0)
    list(APPEND \"\${CMAKE_FIND_PACKAGE_NAME}_${_vtk_export_target_name}_NOT_FOUND_MESSAGE\"
      \"Failed to find the ${_vtk_export_package} package.\")
  endif ()\n")

      string(APPEND _vtk_export_module_build_content "${_vtk_export_module_content}")
      # Private usages should be guarded by `$<BUILD_INTERFACE>` and can be
      # skipped for the install tree regardless of the build mode.
      if (NOT _vtk_export_private)
        string(APPEND _vtk_export_module_install_content "${_vtk_export_module_content}")
      endif ()
    endforeach ()

    set(_vtk_export_module_trailer
"endif ()

unset(_vtk_module_find_package_fail_if_not_found)
unset(_vtk_module_find_package_enabled)
unset(_vtk_module_find_package_required)\n\n")

    if (_vtk_export_module_build_content)
      string(APPEND _vtk_export_build_content
        "${_vtk_export_module_prelude}${_vtk_export_module_build_content}${_vtk_export_module_trailer}")
    endif ()
    if (_vtk_export_module_install_content)
      string(APPEND _vtk_export_install_content
        "${_vtk_export_module_prelude}${_vtk_export_module_install_content}${_vtk_export_module_trailer}")
    endif ()
  endforeach ()

  set(_vtk_export_trailer
    "unset(_vtk_module_find_package_components)
unset(_vtk_module_find_package_components_required)
unset(_vtk_module_find_package_quiet)\n")

  set(_vtk_export_build_file
    "${CMAKE_BINARY_DIR}/${_vtk_export_CMAKE_DESTINATION}/${_vtk_export_FILE_NAME}")
  set(_vtk_export_install_file
    "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_export_FILE_NAME}.install")
  if (_vtk_export_build_content)
    file(WRITE "${_vtk_export_build_file}"
      "${_vtk_export_prelude}${_vtk_export_build_content}${_vtk_export_trailer}")
  else ()
    file(WRITE "${_vtk_export_build_file}" "")
  endif ()
  if (_vtk_export_install_content)
    file(WRITE "${_vtk_export_install_file}"
      "${_vtk_export_prelude}${_vtk_export_install_content}${_vtk_export_trailer}")
  else ()
    file(WRITE "${_vtk_export_install_file}" "")
  endif ()

  install(
    FILES       "${_vtk_export_install_file}"
    DESTINATION "${_vtk_export_CMAKE_DESTINATION}"
    RENAME      "${_vtk_export_FILE_NAME}"
    COMPONENT   "${_vtk_export_COMPONENT}")
endfunction ()

#[==[.rst:
.. _module-third-party:

Third party support
===================
|module|

The module system acknowledges that third party support is a pain and offers
APIs to help wrangle them. Sometimes third party code needs a shim introduced
to make it behave better, so an ``INTERFACE`` library to add that in is very
useful. Other times, third party code is hard to ensure that it exists
everywhere, so it is bundled. When that happens, the ability to select between
the bundled copy and an external copy is useful. All three (and more) of these
are possible.

The following functions are used to handle third party modules:

- :cmake:command:`vtk_module_third_party`
- :cmake:command:`vtk_module_third_party_external`
- :cmake:command:`vtk_module_third_party_internal`
#]==]

#[==[.rst:
.. cmake:command:: vtk_module_third_party

  Third party module.|module|

  When a project has modules which represent third party packages, there are some
  convenience functions to help deal with them. First, there is the meta-wrapper:

  .. code-block:: cmake

    vtk_module_third_party(
      [INTERNAL <internal arguments>...]
      [EXTERNAL <external arguments>...])

  This offers a cache variable named ``VTK_MODULE_USE_EXTERNAL_<module name>`` that
  may be set to trigger between the internal copy and an externally provided
  copy. This is available as a local variable named
  ``VTK_MODULE_USE_EXTERNAL_<library name>``. See the
  :cmake:command:`vtk_module_third_party_external` and :cmake:command`vtk_module_third_party_internal`
  functions for the arguments supported by the ``EXTERNAL`` and ``INTERNAL``
  arguments, respectively.
#]==]
function (vtk_module_third_party)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_third_party
    ""
    ""
    "INTERNAL;EXTERNAL")

  if (_vtk_third_party_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_third_party: "
      "${_vtk_third_party_UNPARSED_ARGUMENTS}")
  endif ()

  string(REPLACE "::" "_" _vtk_build_module_safe "${_vtk_build_module}")
  option("VTK_MODULE_USE_EXTERNAL_${_vtk_build_module_safe}"
    "Use externally provided ${_vtk_build_module}"
    "${_vtk_build_USE_EXTERNAL}")
  mark_as_advanced("VTK_MODULE_USE_EXTERNAL_${_vtk_build_module_safe}")
  get_property(_vtk_third_party_library_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_library_name")
  set("VTK_MODULE_USE_EXTERNAL_${_vtk_third_party_library_name}"
    "${VTK_MODULE_USE_EXTERNAL_${_vtk_build_module_safe}}"
    PARENT_SCOPE)

  if (VTK_MODULE_USE_EXTERNAL_${_vtk_build_module_safe})
    vtk_module_third_party_external(${_vtk_third_party_EXTERNAL})

    # Bubble up variables again.
    foreach (_vtk_third_party_variable IN LISTS _vtk_third_party_variables)
      set("${_vtk_third_party_variable}"
        "${${_vtk_third_party_variable}}"
        PARENT_SCOPE)
    endforeach ()
  else ()
    set(_vtk_third_party_has_external_support 1)
    vtk_module_third_party_internal(${_vtk_third_party_INTERNAL})
  endif ()
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_mark_third_party

  Mark a module as being third party. |module-impl|

  Mark a module as being a third party module.

  .. code-block:: cmake

    _vtk_module_mark_third_party(<target>)

#]==]
function (_vtk_module_mark_third_party target)
  # TODO: `_vtk_module_set_module_property` instead.
  set_target_properties("${target}"
    PROPERTIES
      "INTERFACE_vtk_module_exclude_wrap"    1
      "INTERFACE_vtk_module_include_marshal" 0
      "INTERFACE_vtk_module_third_party"     1)
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_third_party_external

  External third party package. |module|

  A third party dependency may be expressed as a module using this function.
  Third party packages are found using CMake's ``find_package`` function. It is
  highly recommended that imported targets are used to make usage easier. The
  module itself will be created as an ``INTERFACE`` library which exposes the
  package.

  .. code-block:: cmake

    vtk_module_third_party_external(
      PACKAGE               <package>
      [VERSION              <version>]
      [COMPONENTS           <component>...]
      [OPTIONAL_COMPONENTS  <component>...]
      [TARGETS              <target>...]
      [INCLUDE_DIRS <path-or-variable>...]
      [LIBRARIES    <target-or-variable>...]
      [DEFINITIONS  <variable>...]
      [FORWARD_VERSION_REQ  <MAJOR|MINOR|PATCH|EXACT>]
      [VERSION_VAR          <version-spec>]
      [USE_VARIABLES        <variable>...]
      [CONFIG_MODE]
      [STANDARD_INCLUDE_DIRS])

  Only the ``PACKAGE`` argument is required. The arguments are as follows:

  * ``PACKAGE``: (Required) The name of the package to find.
  * ``VERSION``: If specified, the minimum version of the dependency that must be
    found.
  * ``COMPONENTS``: The list of components to request from the package.
  * ``OPTIONAL_COMPONENTS``: The list of optional components to request from the
    package.
  * ``TARGETS``: The list of targets to search for when using this package.
    Targets which do not exist will be ignored to support different versions of
    a package using different target names.
  * ``STANDARD_INCLUDE_DIRS``: If present, standard include directories will be
    added to the module target. This is usually only required if both internal
    and external are supported for a given dependency.
  * ``INCLUDE_DIRS``: If specified, this is added as a ``SYSTEM INTERFACE`` include
    directory for the target. If a variable name is given, it will be
    dereferenced.
  * ``LIBRARIES``: The libraries to link from the package. If a variable name is
    given, it will be dereferenced, however a warning that imported targets are
    not being used will be emitted.
  * ``DEFINITIONS``: If specified, the given variables will be added to the
    target compile definitions interface.
  * ``CONFIG_MODE``: Force ``CONFIG`` mode.
  * ``FORWARD_VERSION_REQ`` and ``VERSION_VAR``: See documentation for
    :cmake:command:`vtk_module_find_package`.
  * ``USE_VARIABLES``: List of variables from the ``find_package`` to make
    available to the caller.
#]==]
function (vtk_module_third_party_external)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_third_party_external
    "STANDARD_INCLUDE_DIRS;CONFIG_MODE"
    "VERSION;PACKAGE;FORWARD_VERSION_REQ;VERSION_VAR"
    "COMPONENTS;OPTIONAL_COMPONENTS;LIBRARIES;INCLUDE_DIRS;DEFINITIONS;TARGETS;USE_VARIABLES")

  if (_vtk_third_party_external_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_third_party_external: "
      "${_vtk_third_party_external_UNPARSED_ARGUMENTS}")
  endif ()

  get_property(_vtk_third_party_external_is_third_party GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_module}_third_party")
  if (NOT _vtk_third_party_external_is_third_party)
    message(FATAL_ERROR
      "The ${_vtk_build_module} has not been declared as a third party "
      "module.")
  endif ()

  if (NOT DEFINED _vtk_third_party_external_PACKAGE)
    message(FATAL_ERROR
      "The `PACKAGE` argument is required.")
  endif ()

  set(_vtk_third_party_external_args)
  if (DEFINED _vtk_third_party_external_FORWARD_VERSION_REQ)
    list(APPEND _vtk_third_party_external_args
      FORWARD_VERSION_REQ "${_vtk_third_party_external_FORWARD_VERSION_REQ}")
  endif ()
  if (DEFINED _vtk_third_party_external_VERSION_VAR)
    list(APPEND _vtk_third_party_external_args
      VERSION_VAR "${_vtk_third_party_external_VERSION_VAR}")
  endif ()

  if (_vtk_third_party_external_TARGETS)
    set(_vtk_third_party_external_config_mode)
    if (_vtk_third_party_external_CONFIG_MODE)
      set(_vtk_third_party_external_config_mode "CONFIG_MODE")
    endif ()

    # If we have targets, they must be exported to the install as well.
    vtk_module_find_package(
      PACKAGE             "${_vtk_third_party_external_PACKAGE}"
      VERSION             "${_vtk_third_party_external_VERSION}"
      COMPONENTS          ${_vtk_third_party_external_COMPONENTS}
      OPTIONAL_COMPONENTS ${_vtk_third_party_external_OPTIONAL_COMPONENTS}
      ${_vtk_third_party_external_config_mode}
      ${_vtk_third_party_external_args})
  else ()
    set(_vtk_third_party_external_config)
    if (_vtk_third_party_external_CONFIG_MODE)
      set(_vtk_third_party_external_config "CONFIG")
    endif ()

    # If there are no targets, the install uses strings and therefore does not
    # need to find the dependency again.
    find_package("${_vtk_third_party_external_PACKAGE}"
      ${_vtk_third_party_external_VERSION}
      ${_vtk_third_party_external_config}
      COMPONENTS          ${_vtk_third_party_external_COMPONENTS}
      OPTIONAL_COMPONENTS ${_vtk_third_party_external_OPTIONAL_COMPONENTS})
  endif ()

  get_property(_vtk_third_party_external_target_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_target_name")

  # Check if an imported target of the same name already exists.
  set(_vtk_third_party_external_real_target_name
    "${_vtk_third_party_external_target_name}")
  set(_vtk_third_party_external_using_mangled_name OFF)
  if (TARGET "${_vtk_third_party_external_target_name}")
    # Ensure that the target collision comes from an imported target.
    get_property(_vtk_third_party_external_is_imported
      TARGET    "${_vtk_third_party_external_target_name}"
      PROPERTY  IMPORTED)
    if (NOT _vtk_third_party_external_is_imported)
      message(FATAL_ERROR
        "It appears as though there is a conflicting target named "
        "`${_vtk_third_party_external_target_name}` expected to be used by "
        "the `${_vtk_build_module}` module already added to the build. This "
        "conflicts with the target name expected to be used by an external "
        "third party dependency.")
    endif ()

    # If it does, we need to have a module name that is not the same as this
    # one. Error out if this is detected.
    if (_vtk_build_module STREQUAL _vtk_third_party_external_target_name)
      message(FATAL_ERROR
        "An imported target has the same name used by the module system for "
        "the facade of the external dependency for `${_vtk_build_module}`. "
        "This module must be either renamed or placed into a namespace.")
    endif ()

    # Mangle the internal name. The alias is the expected use case anyways and
    # since this is an INTERFACE target, there's nothing to break with respect
    # to `make $target` anyways.
    string(APPEND _vtk_third_party_external_real_target_name
      "_vtk_module_mangle")
    set_property(GLOBAL APPEND_STRING
      PROPERTY "_vtk_module_${_vtk_build_module}_target_name"
      "_vtk_module_mangle")
    set(_vtk_third_party_external_using_mangled_name ON)
  endif ()

  add_library("${_vtk_third_party_external_real_target_name}" INTERFACE)
  if (_vtk_third_party_external_using_mangled_name)
    set_property(TARGET "${_vtk_third_party_external_real_target_name}"
      PROPERTY
        EXPORT_NAME "${_vtk_third_party_external_target_name}")
  endif ()
  if (NOT _vtk_build_module STREQUAL _vtk_third_party_external_target_name)
    add_library("${_vtk_build_module}" ALIAS
      "${_vtk_third_party_external_real_target_name}")
  endif ()

  if (_vtk_third_party_external_STANDARD_INCLUDE_DIRS)
    _vtk_module_standard_includes(TARGET "${_vtk_third_party_external_real_target_name}"
      SYSTEM INTERFACE)
  endif ()

  # Try to use targets if they're specified and available.
  set(_vtk_third_party_external_have_targets FALSE)
  set(_vtk_third_party_external_used_targets FALSE)
  if (_vtk_third_party_external_TARGETS)
    set(_vtk_third_party_external_have_targets TRUE)
    foreach (_vtk_third_party_external_target IN LISTS _vtk_third_party_external_TARGETS)
      if (TARGET "${_vtk_third_party_external_target}")
        target_link_libraries("${_vtk_third_party_external_real_target_name}"
          INTERFACE
            "${_vtk_third_party_external_target}")
        set(_vtk_third_party_external_used_targets TRUE)
      endif ()
    endforeach ()
  endif ()

  if (NOT _vtk_third_party_external_used_targets)
    if (NOT _vtk_third_party_external_have_targets)
      message(WARNING
        "A third party dependency for ${_vtk_build_module} was found externally "
        "using paths rather than targets; it is recommended to use imported "
        "targets rather than find_library and such.")
    endif ()

    set(_vtk_third_party_external_have_includes FALSE)
    foreach (_vtk_third_party_external_include_dir IN LISTS _vtk_third_party_external_INCLUDE_DIRS)
      if (DEFINED "${_vtk_third_party_external_include_dir}")
        if (${_vtk_third_party_external_include_dir})
          set(_vtk_third_party_external_have_includes TRUE)
        endif ()
        target_include_directories("${_vtk_third_party_external_real_target_name}" SYSTEM
          INTERFACE "${${_vtk_third_party_external_include_dir}}")
      endif ()
    endforeach ()

    if (_vtk_third_party_external_have_targets AND
        NOT _vtk_third_party_external_have_includes)
      message(WARNING
        "A third party dependency for ${_vtk_build_module} has external targets "
        "which were not found and no `INCLUDE_DIRS` were found either. "
        "Including this module may not work.")
    endif ()

    foreach (_vtk_third_party_external_define IN LISTS _vtk_third_party_external_DEFINITIONS)
      if (DEFINED "${_vtk_third_party_external_define}")
        target_compile_definitions("${_vtk_third_party_external_real_target_name}"
          INTERFACE "${${_vtk_third_party_external_define}}")
      endif ()
    endforeach ()

    set(_vtk_third_party_external_have_libraries FALSE)
    foreach (_vtk_third_party_external_library IN LISTS _vtk_third_party_external_LIBRARIES)
      if (DEFINED "${_vtk_third_party_external_library}")
        if (${_vtk_third_party_external_library})
          set(_vtk_third_party_external_have_libraries TRUE)
        endif ()
        target_link_libraries("${_vtk_third_party_external_real_target_name}"
          INTERFACE "${${_vtk_third_party_external_library}}")
      endif ()
    endforeach ()

    if (_vtk_third_party_external_have_targets AND
        NOT _vtk_third_party_external_have_libraries)
      message(WARNING
        "A third party dependency for ${_vtk_build_module} has external targets "
        "which were not found and no `LIBRARIES` were found either. Linking to "
        "this this module may not work.")
    endif ()
  endif ()

  if (DEFINED _vtk_third_party_external_USE_VARIABLES)
    # If we're called from `vtk_module_third_party`, the variables need bubbled
    # up again.
    if (DEFINED _vtk_third_party_EXTERNAL)
      set(_vtk_third_party_variables
        "${_vtk_third_party_external_USE_VARIABLES}"
        PARENT_SCOPE)
    endif ()

    foreach (_vtk_third_party_external_variable IN LISTS _vtk_third_party_external_USE_VARIABLES)
      if (NOT DEFINED "${_vtk_third_party_external_variable}")
        message(FATAL_ERROR
          "The variable `${_vtk_third_party_external_variable}` was expected "
          "to have been available, but was not defined.")
      endif ()

      set("${_vtk_third_party_external_variable}"
        "${${_vtk_third_party_external_variable}}"
        PARENT_SCOPE)
    endforeach ()
  endif ()

  _vtk_module_mark_third_party("${_vtk_third_party_external_real_target_name}")
  _vtk_module_install("${_vtk_third_party_external_real_target_name}")
endfunction ()

#[==[.rst:
.. cmake:command:: vtk_module_third_party_internal

  Internal third party package. |module|

  Third party modules may also be bundled with the project itself. In this case,
  it is an internal third party dependency. The dependency is assumed to be in a
  subdirectory that will be used via ``add_subdirectory``. Unless it is marked as
  ``HEADERS_ONLY``, it is assumed that it will create a target with the name of the
  module.

  SPDX generation requires that ``SPDX_LICENSE_IDENTIFIER`` and ``SPDX_COPYRIGHT_TEXT``
  are specified.

  .. code-block:: cmake

    vtk_module_third_party_internal(
      [SUBDIRECTORY   <path>]
      [HEADERS_SUBDIR <subdir>]
      [LICENSE_FILES  <file>...]
      [VERSION        <version>]
      [HEADER_ONLY]
      [INTERFACE]
      [STANDARD_INCLUDE_DIRS])

  All arguments are optional, however warnings are emitted if ``LICENSE_FILES``,
  ``VERSION``, ``SPDX_LICENSE_IDENTIFIER`` or ``SPDX_COPYRIGHT_TEXT`` are not specified.

  They are as follows:

  * ``SUBDIRECTORY``: (Defaults to the library name of the module) The
    subdirectory containing the ``CMakeLists.txt`` for the dependency.
  * ``HEADERS_SUBDIR``: If non-empty, the subdirectory to use for installing
    headers.
  * ``LICENSE_FILES``: A list of license files to install for the dependency. If
    not given, a warning will be emitted.
  * ``SPDX_LICENSE_IDENTIFIER``: A license identifier for SPDX file generation
  * ``SPDX_DOWNLOAD_LOCATION``: A download location for SPDX file generation
  * ``SPDX_COPYRIGHT_TEXT``: A copyright text for SPDX file generation
  * ``SPDX_CUSTOM_LICENSE_FILE``: A relative path to a  single custom license file to include in generated SPDX file.
  * ``SPDX_CUSTOM_LICENSE_NAME``: The name of the single custom license, without the ``LicenseRef-``
  * ``VERSION``: The version of the library that is included.
  * ``HEADER_ONLY``: The dependency is header only and will not create a target.
  * ``INTERFACE``: The dependency is an ``INTERFACE`` library.
  * ``STANDARD_INCLUDE_DIRS``: If present, module-standard include directories
    will be added to the module target.
#]==]
function (vtk_module_third_party_internal)
  # TODO: Support scanning for third-party modules which don't support an
  # external copy.

  cmake_parse_arguments(PARSE_ARGV 0 _vtk_third_party_internal
    "INTERFACE;HEADER_ONLY;STANDARD_INCLUDE_DIRS"
    "SUBDIRECTORY;HEADERS_SUBDIR;VERSION;SPDX_DOWNLOAD_LOCATION;SPDX_CUSTOM_LICENSE_FILE;SPDX_CUSTOM_LICENSE_NAME"
    "LICENSE_FILES;SPDX_LICENSE_IDENTIFIER;SPDX_COPYRIGHT_TEXT")

  if (_vtk_third_party_internal_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unparsed arguments for vtk_module_third_party_internal: "
      "${_vtk_third_party_internal_UNPARSED_ARGUMENTS}")
  endif ()

  get_property(_vtk_third_party_internal_is_third_party GLOBAL
    PROPERTY  "_vtk_module_${_vtk_build_module}_third_party")
  if (NOT _vtk_third_party_internal_is_third_party)
    message(FATAL_ERROR
      "The ${_vtk_build_module} has not been declared as a third party "
      "module.")
  endif ()

  get_property(_vtk_third_party_internal_library_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_library_name")
  if (NOT DEFINED _vtk_third_party_internal_SUBDIRECTORY)
    set(_vtk_third_party_internal_SUBDIRECTORY "${_vtk_third_party_internal_library_name}")
  endif ()

  if (NOT DEFINED _vtk_third_party_internal_LICENSE_FILES)
    message(WARNING
      "The ${_vtk_build_module} third party package is embedded, but does not "
      "specify any license files.")
  endif ()

  if (NOT DEFINED _vtk_third_party_internal_VERSION)
    message(WARNING
      "The ${_vtk_build_module} third party package is embedded, but does not "
      "specify the version it is based on.")
  endif ()

  get_property(_vtk_third_party_internal_target_name GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_target_name")
  set(_vtk_third_party_internal_include_type)
  if (_vtk_third_party_internal_INTERFACE)
    set(_vtk_third_party_internal_include_type INTERFACE)
  elseif (_vtk_third_party_internal_HEADER_ONLY)
    add_library("${_vtk_third_party_internal_target_name}" INTERFACE)
    if (NOT _vtk_build_module STREQUAL _vtk_third_party_internal_target_name)
      add_library("${_vtk_build_module}" ALIAS
        "${_vtk_third_party_internal_target_name}")
    endif ()
    set(_vtk_third_party_internal_include_type INTERFACE)
    set(_vtk_third_party_internal_STANDARD_INCLUDE_DIRS 1)
  endif ()

  add_subdirectory("${_vtk_third_party_internal_SUBDIRECTORY}")

  if (NOT TARGET "${_vtk_build_module}")
    message(FATAL_ERROR
      "The ${_vtk_build_module} is being built as an internal third party "
      "library, but a matching target was not created.")
  endif ()

  if (_vtk_third_party_internal_STANDARD_INCLUDE_DIRS)
    _vtk_module_standard_includes(
      TARGET "${_vtk_third_party_internal_target_name}"
      SYSTEM ${_vtk_third_party_internal_include_type}
      HEADERS_DESTINATION "${_vtk_build_HEADERS_DESTINATION}/${_vtk_third_party_internal_HEADERS_SUBDIR}")
  endif ()

  _vtk_module_apply_properties("${_vtk_third_party_internal_target_name}")
  if (_vtk_third_party_internal_INTERFACE)
    # Nothing.
  elseif (_vtk_third_party_internal_HEADER_ONLY)
    _vtk_module_install("${_vtk_third_party_internal_target_name}")
  endif ()

  if (_vtk_third_party_internal_LICENSE_FILES)
    set(_vtk_third_party_internal_license_component "license")
    if (_vtk_build_TARGET_SPECIFIC_COMPONENTS)
      string(PREPEND _vtk_third_party_internal_license_component "${_vtk_build_module}-")
    endif ()
    install(
      FILES       ${_vtk_third_party_internal_LICENSE_FILES}
      DESTINATION "${_vtk_build_LICENSE_DESTINATION}/${_vtk_third_party_internal_library_name}/"
      COMPONENT   "${_vtk_third_party_internal_license_component}")
  endif ()

  if (_vtk_build_GENERATE_SPDX)
    # SPDX_DOWNLOAD_LOCATION is expected for third parties.
    if (NOT _vtk_third_party_internal_SPDX_DOWNLOAD_LOCATION)
      message(AUTHOR_WARNING
        "The ${_vtk_third_party_internal_target_name} module should have a non-empty `SPDX_DOWNLOAD_LOCATION`. Defaulting to NOASSERTION.")
      set(_vtk_third_party_internal_SPDX_DOWNLOAD_LOCATION "NOASSERTION")
    endif ()

    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_build_module}_spdx_license_identifier" "${_vtk_third_party_internal_SPDX_LICENSE_IDENTIFIER}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_build_module}_spdx_copyright_text" "${_vtk_third_party_internal_SPDX_COPYRIGHT_TEXT}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_build_module}_spdx_download_location" "${_vtk_third_party_internal_SPDX_DOWNLOAD_LOCATION}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_build_module}_spdx_custom_license_file" "${_vtk_third_party_internal_SPDX_CUSTOM_LICENSE_FILE}")
    set_property(GLOBAL
      PROPERTY
        "_vtk_module_${_vtk_build_module}_spdx_custom_license_name" "${_vtk_third_party_internal_SPDX_CUSTOM_LICENSE_NAME}")

    _vtk_module_generate_spdx(
      MODULE_NAME "${_vtk_third_party_internal_target_name}"
      TARGET "${_vtk_third_party_internal_target_name}-spdx"
      OUTPUT "${_vtk_third_party_internal_library_name}.spdx")
    add_dependencies("${_vtk_third_party_internal_target_name}" "${_vtk_third_party_internal_target_name}-spdx")

    set(_vtk_third_party_internal_spdx_component "spdx")
    if (_vtk_build_TARGET_SPECIFIC_COMPONENTS)
      string(PREPEND _vtk_third_party_internal_spdx_component "${_vtk_build_module}-")
    endif ()
    install(
      FILES       "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_third_party_internal_library_name}.spdx"
      DESTINATION "${_vtk_build_SPDX_DESTINATION}"
      COMPONENT   "${_vtk_third_party_internal_spdx_component}")
  endif ()

  _vtk_module_mark_third_party("${_vtk_third_party_internal_target_name}")
endfunction ()

#[==[.rst:
.. cmake:command:: _vtk_module_generate_spdx

  SPDX file generation at build time.
  |module-internal|

  Modules can specify a copyright and a license identifier as well as other information
  to generate a SPDX file in order to provide a Software Bill Of Materials (SBOM).
  Inputs files can be parsed for SPDX copyrights and license identifier to add to the
  SPDX file as well.

  .. code-block:: cmake

    _vtk_module_generate_spd(
      [MODULE_NAME <name>]
      [TARGET      <target>]
      [OUTPUT      <file>]
      [SKIP_REGEX  <regex>]
      [INPUT_FILES <file>...]

  All arguments are required except for ``INPUT_FILES``.

  * ``MODULE_NAME``: The name of the module that will be used as package name
    in the SPDX file.
  * ``TARGET``: A CMake target for the generation of the SPDX file at build time
  * ``OUTPUT``: Path to the SPDX file to generate
  * ``SKIP_REGEX``: A python regex to exclude certain source files from SPDX parsing
  * ``INPUT_FILES``: A list of input files to parse for SPDX copyrights and license identifiers,
    some files are automatically excluded from parsing.
#]==]

function (_vtk_module_generate_spdx)
  cmake_parse_arguments(PARSE_ARGV 0 _vtk_module_generate_spdx
    ""
    "MODULE_NAME;TARGET;OUTPUT;SKIP_REGEX"
    "INPUT_FILES")
  if (_vtk_module_generate_spdx_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Unrecognized arguments for _vtk_module_generate_spdx: "
      "${_vtk_module_generate_spdx_UNPARSED_ARGUMENTS}.")
  endif ()
  if (NOT DEFINED _vtk_module_generate_spdx_MODULE_NAME)
    message(FATAL_ERROR
      "The `MODULE_NAME` argument is required.")
  endif ()
  if (NOT DEFINED _vtk_module_generate_spdx_TARGET)
    message(FATAL_ERROR
      "The `TARGET` argument is required.")
  endif ()
  if (NOT DEFINED _vtk_module_generate_spdx_OUTPUT)
    message(FATAL_ERROR
      "The `OUTPUT` argument is required.")
  endif ()

  if (NOT TARGET "Python3::Interpreter")
    find_package(Python3 3.7.0 QUIET COMPONENTS Interpreter)
    if (NOT Python3_FOUND)
      message(WARNING
        "Python (>= 3.7.0) not found, skipping SPDX generation for "
        "${_vtk_module_generate_spdx_MODULE_NAME}")
      return ()
    endif ()
  endif ()
  if (NOT DEFINED Python3_VERSION)
    message(WARNING
      "Unknown Python3 version; SPDX generation requires at least 3.7.")
  elseif (Python3_VERSION VERSION_LESS "3.7.0")
    message(WARNING
      "SPDX generation for '${_vtk_module_generate_spdx_MODULE_NAME}' "
      "requires at least Python 3.7 (found ${Python3_VERSION}). Skipping.")
    return ()
  endif ()

  set(_vtk_module_generate_spdx_output_file "${CMAKE_CURRENT_BINARY_DIR}/${_vtk_module_generate_spdx_OUTPUT}")
  string(TIMESTAMP _vtk_module_generate_spdx_timestamp UTC)

  get_property(_vtk_module_generate_spdx_SPDX_LICENSE_IDENTIFIER GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_spdx_license_identifier")
  if (NOT _vtk_module_generate_spdx_SPDX_LICENSE_IDENTIFIER)
    message(AUTHOR_WARNING
      "The ${_vtk_module_generate_spdx_MODULE_NAME} module should have a "
      "non-empty `SPDX_LICENSE_IDENTIFIER`. Defaulting to NOASSERTION.")
    set(_vtk_module_generate_spdx_SPDX_LICENSE_IDENTIFIER "NOASSERTION")
  endif ()

  get_property(_vtk_module_generate_spdx_SPDX_COPYRIGHT_TEXT GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_spdx_copyright_text")
  if (NOT _vtk_module_generate_spdx_SPDX_COPYRIGHT_TEXT)
    message(AUTHOR_WARNING
      "The ${_vtk_module_generate_spdx_MODULE_NAME} module should have a "
      "non-empty `SPDX_COPYRIGHT_TEXT`. Defaulting to NOASSERTION")
    set(_vtk_module_generate_spdx_SPDX_COPYRIGHT_TEXT "NOASSERTION")
  endif ()

  get_property(_vtk_module_generate_spdx_SPDX_CUSTOM_LICENSE_FILE GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_spdx_custom_license_file")
  get_property(_vtk_module_generate_spdx_SPDX_CUSTOM_LICENSE_NAME GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_spdx_custom_license_name")

  if (NOT _vtk_build_SPDX_DOCUMENT_NAMESPACE)
    message(AUTHOR_WARNING
      "_vtk_build_SPDX_DOCUMENT_NAMESPACE variable is not defined, defaulting to https://vtk.org/spdx")
    set(_vtk_module_generate_spdx_namespace "https://vtk.org/spdx")
  endif ()
  set(_vtk_module_generate_spdx_namespace "${_vtk_build_SPDX_DOCUMENT_NAMESPACE}/${_vtk_module_generate_spdx_MODULE_NAME}")

  get_property(_vtk_module_generate_spdx_download_location GLOBAL
    PROPERTY "_vtk_module_${_vtk_build_module}_spdx_download_location")
  if (NOT _vtk_module_generate_spdx_download_location)
    if (NOT _vtk_build_SPDX_DOWNLOAD_LOCATION)
      message(AUTHOR_WARNING
        "_vtk_build_SPDX_DOWNLOAD_LOCATION variable is not defined, defaulting to NOASSERTION")
      set(_vtk_module_generate_spdx_download_location "NOASSERTION")
    else ()
      string(REPLACE "${CMAKE_SOURCE_DIR}" "" _vtk_module_generate_spdx_download_location_suffix "${CMAKE_CURRENT_SOURCE_DIR}")
      set(_vtk_module_generate_spdx_download_location "${_vtk_build_SPDX_DOWNLOAD_LOCATION}${_vtk_module_generate_spdx_download_location_suffix}")
    endif ()
  endif ()

  set(_vtk_module_generate_spdx_args_file)
  set(_vtk_module_generate_spdx_response_arg)
  set(_vtk_module_generate_spdx_input_paths)

  if (_vtk_module_generate_spdx_INPUT_FILES)
    foreach (_vtk_module_generate_spdx_input_file IN LISTS _vtk_module_generate_spdx_INPUT_FILES)
      if (IS_ABSOLUTE "${_vtk_module_generate_spdx_input_file}")
        list(APPEND _vtk_module_generate_spdx_input_paths
          "${_vtk_module_generate_spdx_input_file}")
      else ()
        list(APPEND _vtk_module_generate_spdx_input_paths
          "${CMAKE_CURRENT_SOURCE_DIR}/${_vtk_module_generate_spdx_input_file}")
      endif ()
    endforeach ()

    set(_vtk_module_generate_spdx_args_file
      "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_vtk_module_generate_spdx_TARGET}/${_vtk_module_generate_spdx_MODULE_NAME}-spdx.$<CONFIGURATION>.args")
    set(_vtk_module_generate_spdx_response_arg "@${_vtk_module_generate_spdx_args_file}")
    file(GENERATE
      OUTPUT  "${_vtk_module_generate_spdx_args_file}"
      CONTENT "$<JOIN:${_vtk_module_generate_spdx_input_paths},\n>")
  endif ()

  add_custom_command(
    OUTPUT "${_vtk_module_generate_spdx_output_file}"
    COMMAND "$<TARGET_FILE:Python3::Interpreter>" -Xutf8 "${_vtkModule_dir}/SPDX_generate_output.py"
      -m "${_vtk_module_generate_spdx_MODULE_NAME}"
      -l "${_vtk_module_generate_spdx_SPDX_LICENSE_IDENTIFIER}"
      -c "${_vtk_module_generate_spdx_SPDX_COPYRIGHT_TEXT}"
      -x "${_vtk_module_generate_spdx_SPDX_CUSTOM_LICENSE_FILE}"
      -y "${_vtk_module_generate_spdx_SPDX_CUSTOM_LICENSE_NAME}"
      -o "${_vtk_module_generate_spdx_output_file}"
      -s "${CMAKE_CURRENT_SOURCE_DIR}"
      -n "${_vtk_module_generate_spdx_namespace}"
      -d "${_vtk_module_generate_spdx_download_location}"
      -k "${_vtk_module_generate_spdx_SKIP_REGEX}"
      ${_vtk_module_generate_spdx_response_arg}
    DEPENDS ${_vtk_module_generate_spdx_input_paths}
            ${_vtk_module_generate_spdx_args_file}
    VERBATIM)
  add_custom_target("${_vtk_module_generate_spdx_TARGET}"
    DEPENDS
      "${_vtk_module_generate_spdx_output_file}")
endfunction ()
