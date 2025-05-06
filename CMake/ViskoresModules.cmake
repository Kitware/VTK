##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

option(Viskores_VERBOSE_MODULES
  "When on, extra information about what modules are found and why they are \
built or not is added as CMake status messages."
  OFF
  )

mark_as_advanced(Viskores_VERBOSE_MODULES)

# -----------------------------------------------------------------------------

#[==[
Sets a property for a module of a particular name. The module property is
available globally and can be retrieved with `viskores_module_get_property`.
#]==]
function(viskores_module_set_property module_name property_name property_value)
  set_property(GLOBAL PROPERTY
    "_viskores_module_${module_name}_${property_name}" "${property_value}")
endfunction()

#[==[
Gets a property for a value of a particular name.
#]==]
function(viskores_module_get_property out_var module_name property_name)
  get_property(_viskores_module_property GLOBAL PROPERTY
    "_viskores_module_${module_name}_${property_name}")
  set("${out_var}" "${_viskores_module_property}" PARENT_SCOPE)
endfunction()

#[==[
Sets out_var to true if a module of the given name exists, false otherwise.
A module does not need to be built for this to return true. Only the
associated viskores.module file needs to exist.
#]==]
function(viskores_module_exists out_var module_name)
  viskores_module_get_property(_viskores_module_exists_name ${module_name} NAME)
  if(_viskores_module_exists_name)
    set("${out_var}" TRUE PARENT_SCOPE)
  else()
    set("${out_var}" FALSE PARENT_SCOPE)
  endif()
endfunction()

#[==[
Sets out_var to a list of all modules that are being built. This list includes
all modules that are enabled and have an associated target. Any modules not being
built will not be listed.

The modules are listed in an order such that any module will be listed _after_
any modules that it depends on.
#]==]
function(viskores_module_get_list out_var)
  get_property(_viskores_module_list GLOBAL PROPERTY "_viskores_module_list")
  set("${out_var}" "${_viskores_module_list}" PARENT_SCOPE)
endfunction()

#[==[
Forces the enable value of a module group to be a particular value. This is
useful for converting a CMake `option` to a group of modules to turn on or
off. When a group of modules is forced with this function, a CMake cache
variable is not made, thus not allowing the user to change the value.
(Presumably, the value is changed via other, easier means.)

```cmake
viskores_module_force_group(group_name
  [VALUE <value>] | [ENABLE_OPTION <name>] | [DISABLE_OPTION <name>]
  [ENABLE_VALUE <value>]
  [DISABLE_VALUE <value>]
  [REASON <string>]
  )

The first argument is always the name of the group to force. Exactly
one of VALUE, ENABLE_OPTION, or DISABLE_OPTION must be provided.

  * `VALUE`: The specific value to set the group enable/disable flag.
    Must be `YES`, `NO`, `WANT`, `DONT_WANT`, `NO`, or `DEFAULT`.
  * `ENABLE_OPTION`: The name of a CMake variable (usually a cache
    variable) that sets the flag to `ENABLE_VALUE` if true or
    `DISABLE_VALUE` if false.
  * `DISABLE_OPTION`: The name of a CMake variable (usually a cache
    variable) that sets the flag to `DISABLE_VALUE` if true or
    `ENABLE_VALUE` if false.
  * `ENABLE_VALUE`: Value used to turn on the group. Defaults to `YES`.
  * `DISABLE_VALUE`: Value used to turn off the group. Defaults to `NO`.
  * `REASON`: String given to inform users how the group enable/disable
    flag got its value.
```
#]==]
function(viskores_module_force_group group_name)
  cmake_parse_arguments(PARSE_ARGV 1 force
    ""
    "VALUE;ENABLE_OPTION;DISABLE_OPTION;ENABLE_VALUE;DISABLE_VALUE;REASON"
    "")
  if(force_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Bad arguments to viskores_module_force_group")
  endif()
  if(NOT DEFINED force_ENABLE_VALUE)
    set(force_ENABLE_VALUE "YES")
  endif()
  if(NOT DEFINED force_DISABLE_VALUE)
    set(force_DISABLE_VALUE "NO")
  endif()

  if(DEFINED force_VALUE)
    if(NOT force_REASON)
      set(force_REASON "Forced value")
    endif()
  elseif(force_ENABLE_OPTION)
    if(${${force_ENABLE_OPTION}})
      set(force_VALUE ${force_ENABLE_VALUE})
    else()
      set(force_VALUE ${force_DISABLE_VALUE})
    endif()
    if(NOT force_REASON)
      set(force_REASON "${force_ENABLE_OPTION} set to ${${force_ENABLE_OPTION}}")
    endif()
  elseif(force_DISABLE_OPTION)
    if(NOT ${${force_DISABLE_OPTION}})
      set(force_VALUE ${force_ENABLE_VALUE})
    else()
      set(force_VALUE ${force_DISABLE_VALUE})
    endif()
    if(NOT force_REASON)
      set(force_REASON "${force_DISABLE_OPTION} set to ${${force_DISABLE_OPTION}}")
    endif()
  else()
    message(FATAL_ERROR
      "viskores_module_force_group must be given VALUE, ENABLE_OPTION, or DISABLE_OPTION")
  endif()

  set(force_REASON "${force_REASON} (forcing Viskores_GROUP_ENABLE_${group_name} to ${force_VALUE})")

  set_property(GLOBAL PROPERTY
    "_viskores_module_group_enable_${group_name}_override" TRUE)
  set_property(GLOBAL PROPERTY
    "_viskores_module_group_enable_${group_name}_value" "${force_VALUE}")
  set_property(GLOBAL PROPERTY
    "_viskores_module_group_enable_${group_name}_reason" "${force_REASON}")
endfunction()

#[==[
Creates a CMake variable to enable/disable the module with the given name.
This cached variable can be set by the user to turn the module on or off.
If the cache variable already exists, then this call has no effect.
#]==]
function(viskores_module_enable_module_variable module_name default_value)
  set(Viskores_MODULE_ENABLE_${module_name} ${default_value}
    CACHE INTERNAL
    "Enable the ${module_name} module.
    YES - Always create the module (it is an error otherwise).
    WANT - Create the module if possible.
    DONT_WANT - Create the module only if there is a dependency that requires it.
    NO - Never create the module.
    DEFAULT - Do the default behavior."
    )
  mark_as_advanced(Viskores_MODULE_ENABLE_${module_name})
  set_property(CACHE Viskores_MODULE_ENABLE_${module_name}
    PROPERTY STRINGS "YES;WANT;DONT_WANT;NO;DEFAULT"
    )
endfunction()

#[==[
Creates a CMake variable to enable/disable the modules with the given group name.
This cached variable can be set by the user to turn the module on or off.
This cache variable only has an effect on modules that belong to this group and
have their own ENABLE flag set to DEFAULT.
If the cache variable already exists, then this call has no effect.
#]==]
function(viskores_module_enable_group_variable group_name default_value)
  get_property(override_exists GLOBAL PROPERTY _viskores_module_group_enable_${group_name}_override)
  if(override_exists)
    # There is a force of this group overriding any setting, in which case
    # don't even give the option.
    return()
  endif()
  set(Viskores_GROUP_ENABLE_${group_name} ${default_value}
    CACHE INTERNAL
    "Enable the ${group_name} module group.
    YES - Always create the group of modules (it is an error otherwise).
    WANT - Create the group of modules if possible.
    DONT_WANT - Create the module only if there is a dependency that requires it.
    NO - Never create the module.
    DEFAULT - Do the default behavior."
    )
  mark_as_advanced(Viskores_GROUP_ENABLE_${group_name})
  set_property(CACHE Viskores_GROUP_ENABLE_${group_name}
    PROPERTY STRINGS "YES;WANT;DONT_WANT;NO;DEFAULT"
    )
endfunction()

# -----------------------------------------------------------------------------

#[==[
Parses the given `module_file`. The name of the module (extracted from the file)
is returned in `name_var`.

For each module option expected in the module file, a module property with the
same name is created to hold that option. Additionally, a `DIRECTORY` property
is created to point to the source directory containing the module. The module
properties can be retrieved with the viskores_module_get_property function.
#]==]
function(_viskores_module_parse_module_file module_file name_var)
  # Read the file
  if(NOT IS_ABSOLUTE "${module_file}")
    string(PREPEND module_file "${CMAKE_CURRENT_SOURCE_DIR}/")
  endif()
  file(READ ${module_file} module_file_contents)
  # Remove comments
  string(REGEX REPLACE "#[^\n]*\n" "\n" module_file_contents "${module_file_contents}")
  # Separate arguments with `;` to treat it like a list.
  string(REGEX REPLACE "( |\n)+" ";" module_file_contents "${module_file_contents}")

  # Parse module file as arguments to a function
  set(options NO_TESTING)
  set(oneValueArgs NAME TESTING_DIR)
  set(multiValueArgs
    GROUPS DEPENDS PRIVATE_DEPENDS OPTIONAL_DEPENDS TEST_DEPENDS TEST_OPTIONAL_DEPENDS)
  cmake_parse_arguments(_viskores_module
    "${options}"
    "${oneValueArgs}"
    "${multiValueArgs}"
    ${module_file_contents})

  # Check required arguments
  if(_viskores_module_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR
      "Module file ${module_file} contains unknown options: ${_viskores_module_UNPARSED_ARGUMENTS}")
  endif()
  if(NOT _viskores_module_NAME)
    message(FATAL_ERROR "Module file ${module_file} does not specify a NAME option.")
  endif()
  set(NAME ${_viskores_module_NAME})

  # Stash the information in variables
  foreach(module_var_name IN LISTS options oneValueArgs multiValueArgs)
    viskores_module_set_property(${NAME} ${module_var_name} "${_viskores_module_${module_var_name}}")
  endforeach()
  get_filename_component(directory "${module_file}" DIRECTORY)
  viskores_module_set_property(${NAME} DIRECTORY "${directory}")

  # Add a build dependency that reruns CMake whenever the viskores.module file changes.
  set_property(DIRECTORY "${PROJECT_SOURCE_DIR}"
    APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${module_file}")

  set(${name_var} ${NAME} PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------

#[==[
Scans the given directories recursively for `viskores.module` files and put the
paths into the output variable. Note that the module files are assumed to
live next to the `CMakeLists.txt` file, which will build the module.
#]==]
function(_viskores_modules_find output)
  set(all_modules)
  foreach(search_directory IN LISTS ARGN)
    file(GLOB_RECURSE found_modules "${search_directory}/viskores.module")
    list(APPEND all_modules ${found_modules})
  endforeach()
  set(${output} ${all_modules} PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------

#[==[
Scans for modules and builds the appropriate cached variables to optionally
build them. Modules are found by looking for files named `viskores.module`.
This module file is assumed to live next to the `CMakeLists.txt` file that
can build the module.

```cmake
viskores_modules_scan(
  SCAN_DIRECTORIES <file>...
  PROVIDED_MODULES <variable>
  )

The arguments are as follows:

  * `SCAN_DIRECTORIES`: (Required) A list of directories to (recursively) scan
    for modules (indicated by a `viskores.module` file).
  * `PROVIDED_MODULES`: (Required) The name of a variable that will be set with
    a list of modules that exist.

All the directories in `SCAN_DIRECTORIES` are recursively searched for files
named `viskores.module`. See docs/Modules.md for information on the format of
`viskores.module` files.
#]==]
function(viskores_modules_scan)
  cmake_parse_arguments(PARSE_ARGV 0 _scan
    ""
    "PROVIDED_MODULES"
    "SCAN_DIRECTORIES"
    )
  if(_scan_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Invalid arguments given to viskores_module_scan.")
  endif()
  if(NOT _scan_PROVIDED_MODULES)
    message(FATAL_ERROR "viskores_scan_modules must have a `PROVIDED_MODULES` argument.")
  endif()

  _viskores_modules_find(_scan_module_files ${_scan_SCAN_DIRECTORIES})
  if (NOT _scan_module_files)
    message(FATAL_ERROR "No viskores.module files found in '${_scan_SCAN_DIRECTORIES}'")
  endif()

  set(_scan_module_names)
  # Read all of the module files passed in.
  foreach(module_file IN LISTS _scan_module_files)
    _viskores_module_parse_module_file(${module_file} name)
    if(Viskores_VERBOSE_MODULES)
      message(STATUS "Found module ${name} in ${module_file}")
    endif()

    viskores_module_enable_module_variable(${name} "DEFAULT")

    viskores_module_get_property(module_groups ${name} GROUPS)
    foreach(group_name IN LISTS module_groups)
      viskores_module_enable_group_variable(${group_name} "DEFAULT")
    endforeach()

    list(APPEND _scan_module_names "${name}")
  endforeach()

  set(${_scan_PROVIDED_MODULES} ${_scan_module_names} PARENT_SCOPE)
endfunction()

# -----------------------------------------------------------------------------

function(_viskores_module_check_enable_flag variable_name)
  set(valid_flags YES WANT DONT_WANT NO DEFAULT)
  if(NOT "${${variable_name}}" IN_LIST valid_flags)
    message(FATAL_ERROR
      "${variable_name} must be set to one of ${valid_flags}, not `${${variable_name}}`.")
  endif()
endfunction()

function(_viskores_module_get_enable_flag module output)
  set(reasons_output "${ARGV2}")
  _viskores_module_check_enable_flag(Viskores_MODULE_ENABLE_${module})

  set(enable_flag ${Viskores_MODULE_ENABLE_${module}})
  if(reasons_output)
    set(reasons "Viskores_MODULE_ENABLE_${module} is set to `${Viskores_MODULE_ENABLE_${module}}`")
  else()
    set(reasons)
  endif()

  if(enable_flag STREQUAL "DEFAULT")
    viskores_module_get_property(module_groups ${module} GROUPS)
    foreach(group IN LISTS module_groups)
      get_property(override_exists GLOBAL PROPERTY _viskores_module_group_enable_${group}_override)
      if(override_exists)
        get_property(group_enable_flag GLOBAL PROPERTY _viskores_module_group_enable_${group}_value)
        if(reasons_output)
          get_property(force_reason
            GLOBAL PROPERTY _viskores_module_group_enable_${group}_reason)
          list(APPEND reasons "${force_reason}")
        endif()
      else()
        _viskores_module_check_enable_flag(Viskores_GROUP_ENABLE_${group})
        set(group_enable_flag "${Viskores_GROUP_ENABLE_${group}}")
        if(reasons_output)
          list(APPEND reasons "Viskores_GROUP_ENABLE_${group} is set to `${group_enable_flag}`")
        endif()
      endif()
      if(NOT ${group_enable_flag} STREQUAL "DEFAULT")
        set(enable_flag ${group_enable_flag})
        break()
      endif()
    endforeach()
  endif()

  if(enable_flag STREQUAL "DEFAULT")
    if(_viskores_modules_want_by_default)
      set(enable_flag "WANT")
    else()
      set(enable_flag "DONT_WANT")
    endif()
    if(reasons_output)
      list(APPEND reasons "${_viskores_modules_want_by_default_reason}")
    endif()
  endif()

  set(${output} ${enable_flag} PARENT_SCOPE)
  if(reasons_output)
    set(${reasons_output} ${reasons} PARENT_SCOPE)
  endif()
endfunction()

function(_viskores_modules_print_enable_flag module)
  _viskores_module_get_enable_flag(${module} enable_flag reasons)
  message(STATUS "Module ${module} is `${enable_flag}` because:")
  foreach(reason IN LISTS reasons)
    message(STATUS "    ${reason}")
  endforeach()
endfunction()

function(_viskores_modules_print_dependency_chain first_module)
  set(last_module ${first_module})
  foreach(dep IN LISTS ARGN)
    message(STATUS "Module `${last_module}` depends on module `${dep}`")
    set(last_module ${dep})
  endforeach()
endfunction()

# -----------------------------------------------------------------------------

function(_viskores_modules_try_build target_module dependent_module dependency_chain)
  viskores_module_exists(exists ${target_module})
  if(NOT exists)
    # The calling code should check to make sure something is a module before calling this.
    message(FATAL_ERROR "\
Internal error: _viskores_modules_try_build called for a non-existant module `${target_module}`.")
  endif()

  if(TARGET ${target_module})
    # This module is already created. Everything is good.
    return()
  endif()

  # Detect circular dependencies (to prevent infinite CMake loops)
  list(FIND dependency_chain ${target_module} chain_index)
  if(chain_index GREATER -1)
    message("Circular dependency in modules detected!")
    list(SUBLIST dependency_chain ${chain_index} -1 subchain)
    _viskores_modules_print_dependency_chain(${subchain} ${target_module})
    message(FATAL_ERROR "\
Detected a circular dependency for module `${target_module}`. See the previous \
messages for the dependency chain. Modify the dependencies in the viskores.module \
files to break the dependency cycle.")
  endif()

  _viskores_module_get_enable_flag(${target_module} enable_flag)

  if(enable_flag STREQUAL "NO")
    # Cannot build this module.
    if(dependent_module)
      message("\
Unable to enable module ${dependent_module} because module ${depends_on_module}, \
on which it depends, cannot be enabled. See the following status messages \
for more information.")
      _viskores_modules_print_enable_flag(${dependent_module})
      _viskores_modules_print_enable_flag(${target_module})
      _viskores_modules_print_dependency_chain(${dependent_module} ${dependency_chain} ${target_module})
      message(FATAL_ERROR "Inconsistent module enable states. See previous status for details.")
    endif()
    if(Viskores_VERBOSE_MODULES)
      message(STATUS "Not building module ${target_module} because enable flag set to NO.")
      _viskores_modules_print_enable_flag(${target_module})
    endif()
    return()
  endif()

  if((enable_flag STREQUAL "DONT_WANT") AND (NOT dependent_module))
    # Have no reason to build this module.
    if(Viskores_VERBOSE_MODULES)
      message(STATUS "\
Not building module ${target_module} because enable flag set to `DONT_WANT` \
unless a dependency is found.")
      _viskores_modules_print_enable_flag(${target_module})
    endif()
    return()
  endif()

  # At this point, we either want or need the module.

  if((enable_flag STREQUAL "YES") AND (NOT dependent_module))
    # Found new target_module
    set(dependent_module ${target_module})
  endif()
  list(APPEND dependency_chain ${target_module})

  # Attempt to build all dependent modules first.
  viskores_module_get_property(module_dependencies ${target_module} DEPENDS)
  viskores_module_get_property(module_private_dependencies ${target_module} PRIVATE_DEPENDS)
  foreach(depends_on_module IN LISTS module_dependencies module_private_dependencies)
    # A module can depend on either a module (which defines a target) or a target created
    # somewhere else. If it depends on a module, allow that module to create the target.
    viskores_module_exists(depends_on_is_module ${depends_on_module})
    if(depends_on_is_module)
      _viskores_modules_try_build(${depends_on_module} "${dependent_module}" "${dependency_chain}")
    endif()
    if(NOT TARGET ${depends_on_module})
      # The module target_module depends on is not being built. Thus, we cannot compile
      # this module. Customize the messages based on whether this module was needed to
      # be on for correctness and whether the dependency is actually a module or just an
      # expected library.
      if(dependent_module)
        if(depends_on_is_module)
          # Internal error. If the dependent module could not be built, it should have already
          # issued a FATAL_ERROR.
          message(FATAL_ERROR "Internal error: Required module not built.")
        else()
          _viskores_modules_print_enable_flag(${target_module})
          message(FATAL_ERROR "\
Unable to enable module `${target_module}` because it depends on `${depends_on_module}`. \
There is no module of that name. Either create a module of that name (using a `viskores.module` \
file) or create a CMake target of that name before the modules are built.")
        endif()
      elseif(Viskores_VERBOSE_MODULES)
        if(depends_on_is_module)
          message(STATUS "\
Not building module `${target_module}` because it depends on module `${depends_on_module}`, \
which is not built.")
        else()
          message(STATUS "\
Not building module `${target_module}` because it depends on `${depends_on_module}`, which \
is neither a module or an existing target. Either create a module of that name (using a \
`viskores.module` file) or create a CMake target of that name before the modules are built.")
        endif()
        _viskores_modules_print_enable_flag(${target_module})
      endif()
      return()
    endif()
  endforeach()

  # Also attempt to build optional dependent modules. These behave the same except we do
  # not generate an error if the module cannot be loaded by removing the "dependent_module"
  # and ignoring whether the dependent module actually gets built.
  viskores_module_get_property(module_optional_dependencies ${target_module} OPTIONAL_DEPENDS)
  foreach(depends_on_module IN LISTS module_optional_dependencies)
    viskores_module_exists(depends_on_is_module ${depends_on_module})
    if(depends_on_is_module)
      _viskores_modules_try_build(${depends_on_module} "" "${dependency_chain}")
    endif()
  endforeach()

  # We have verified that we can enable this module.

  if(Viskores_VERBOSE_MODULES)
    if(enable_flag STREQUAL "DONT_WANT")
      list(GET dependency_chain -2 needed_by)
      message(STATUS "Enabling module `${target_module}` because it is needed by `${needed_by}`")
    else()
      message(STATUS "Enabling module `${target_module}`")
    endif()
    _viskores_modules_print_enable_flag(${target_module})
  endif()
  viskores_module_get_property(src_directory ${target_module} DIRECTORY)
  file(RELATIVE_PATH rel_directory "${Viskores_SOURCE_DIR}" "${src_directory}")
  set(viskores_module_current ${target_module})
  add_subdirectory("${src_directory}" "${Viskores_BINARY_DIR}/${rel_directory}")
  set(viskores_module_current)
  if(NOT TARGET ${target_module})
    if(Viskores_VERBOSE_MODULES)
      message(STATUS "\
Module `${target_module}` did not create the expected target. Creating a 'fake' target \
so that other modules know this module is loaded.")
    endif()
    add_library(${target_module} INTERFACE)
  endif()
  get_property(_viskores_module_list GLOBAL PROPERTY "_viskores_module_list")
  list(APPEND _viskores_module_list ${target_module})
  set_property(GLOBAL PROPERTY "_viskores_module_list" "${_viskores_module_list}")
  get_property(target_module_type
    TARGET    "${target_module}"
    PROPERTY  TYPE)
  if (target_module_type STREQUAL "SHARED_LIBRARY")
    set_property(TARGET "${target_module}"
      PROPERTY
      BUILD_RPATH_USE_ORIGIN 1)
    if (UNIX)
      if (APPLE)
        set(target_module_rpath
          "@loader_path")
      else ()
        set(target_module_rpath
          "$ORIGIN")
      endif ()
      set_property(TARGET "${target_module}" APPEND
        PROPERTY
        INSTALL_RPATH "${target_module_rpath}")
    endif ()
  endif()
endfunction()

# -----------------------------------------------------------------------------

function(_viskores_modules_try_build_tests target_module)
  if(NOT TARGET ${target_module})
    # This module was never created, so don't compile tests for it.
    return()
  endif()

  viskores_module_get_property(no_testing ${target_module} NO_TESTING)
  if(no_testing)
    if(Viskores_VERBOSE_MODULES)
      message(STATUS
        "Not building tests for `${target_module}` because it has the NO_TESTING option.")
    endif()
    return()
  endif()

  viskores_module_get_property(target_dependencies ${target_module} TEST_DEPENDS)
  foreach(dependency IN LISTS target_dependencies)
    if(NOT TARGET ${dependency})
      if(Viskores_VERBOSE_MODULES)
        message(STATUS
          "Not building tests for `${target_module}` because missing dependency `${dependency}`")
      endif()
      return()
    endif()
  endforeach()

  viskores_module_get_property(testing_subdir ${target_module} TESTING_DIR)
  if(NOT testing_subdir)
    set(testing_subdir "testing")
  endif()

  viskores_module_get_property(src_directory ${target_module} DIRECTORY)
  file(RELATIVE_PATH rel_directory "${Viskores_SOURCE_DIR}" "${src_directory}")
  set(viskores_module_current_test ${target_module})
  add_subdirectory("${src_directory}/${testing_subdir}"
    "${Viskores_BINARY_DIR}/${rel_directory}/${testing_subdir}")
  set(viskores_module_current_test)
endfunction()

# -----------------------------------------------------------------------------

#[==[
Determines which modules should be built and then calls `add_subdirectory` on those
modules to build them.

```cmake
viskores_modules_build(
  PROVIDED_MODULES <name>...
  [WANT_BY_DEFAULT <ON|OFF>]
  [WANT_BY_DEFAULT_REASON <string>]
  )

The arguments are as follows:

  * `PROVIDED_MODULES`: (Required) A list of module names that are available.
    This list typically provided by `viskores_modules_scan`.
  * `WANT_BY_DEFAULT`: (Defaults to `OFF`) Whether modules should by default be
    built if possible.
  * `WANT_BY_DEFAULT_REASON`: When `Viskores_VERBOSE_MODULES` and a module's enable
    is set to the default value (dictated by `WANT_BY_DEFAULT`), then this string
    is given as the reason.
```
#]==]
function(viskores_modules_build)
  cmake_parse_arguments(PARSE_ARGV 0 _build
    ""
    "WANT_BY_DEFAULT;WANT_BY_DEFAULT_REASON"
    "PROVIDED_MODULES"
    )

  if(_build_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Invalid arguments given to viskores_modules_build.")
  endif()
  if(NOT _build_PROVIDED_MODULES)
    message(FATAL_ERROR "viskores_modules_build requires PROVIDED_MODULES variable.")
  endif()

  if(_build_WANT_BY_DEFAULT)
    set(_viskores_modules_want_by_default ${_build_WANT_BY_DEFAULT})
  else()
    set(_viskores_modules_want_by_default OFF)
  endif()
  if(_build_WANT_BY_DEFAULT_REASON)
    set(_viskores_modules_want_by_default_reason "${_build_WANT_BY_DEFAULT_REASON}")
  else()
    set(_viskores_modules_want_by_default_reason "WANT_BY_DEFAULT is ${_viskores_modules_want_by_default}")
  endif()

  foreach(module IN LISTS _build_PROVIDED_MODULES)
    _viskores_modules_try_build(${module} "" "")
  endforeach()

  if(Viskores_ENABLE_TESTING)
    foreach(module IN LISTS _build_PROVIDED_MODULES)
      _viskores_modules_try_build_tests(${module})
    endforeach()
  endif()
endfunction()
