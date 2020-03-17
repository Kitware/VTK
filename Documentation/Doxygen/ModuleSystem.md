# Module System

VTK 9.0 introduces a new build system compared to previous versions. This
version uses CMake's built-in functionality for behaviors that were performed
manually in the previous iteration of the build system.

# Terminology

  - **module**: A unit of API provided by a project. This is the core of the
    system and there are lots of features available through this mechanism that
    are not provided by CMake's library or other usage requirements.
  - **group**: A configure-time collection of modules. These may be used to
    control whether member modules will be built or not with a single flag.
  - **kit**: A collection of modules for which all the compiled code is placed
    in a single library.
  - **property**: An attribute of a module. Only of real interest to developers
    of the module system and its extensions.
  - **autoinit**: A mechanism for triggering registration to global registries
    based on the complete set of linked-to libraries.
  - **third party**: A module representing an external dependency.
  - **enable status**: A 4-way state to allow for "weak" and "strong" selection
    or deselection of a module or group for building.

# Principles

The module system was designed with a number of principles in mind. These
should be followed as much as possible when developing extensions as well.

  - The minimum CMake version required by the module system should be as low
    as possible to get the required features. For example, if a new feature is
    available in 3.15 that improves core module functionality, that'd be a
    reasonable reason to require it. But a bugfix in 3.10 that can be worked
    around should not bump the minimum version. Currently CMake 3.8 is
    expected to work, though various features (such as kits) are only
    available with newer CMake versions.
  - Build tree looks like the install tree. The layout of the build tree is set
    up to mirror the layout of the install tree. This allows more code content
    to be shared between build and install time.
  - Convention over configuration. CMake conventions should be followed. Of
    note, projects are assumed to be "well-behaved" including, but not limited
    to:
      - use of [`BUILD_SHARED_LIBS`][BUILD_SHARED_LIBS] to control shared vs.
        static library compilation;
      - use of [`GNUInstallDirs`][GNUInstallDirs]; and
      - sensible defaults based on things like
        [`CMAKE_PROJECT_NAME`][cmake-CMAKE_PROJECT_NAME] as set by the
        [`project()`][cmake-project] function.
  - Configuration through API. Where configuration is provided, instead of
    using global state or "magic" variables, configuration should be provided
    through parameters to the API functions provided. Concessions are made for
    rarely-used functionality or where the API would be complicated to plumb
    through the required information. These variables (which are typically
    parameterized) are documented at the end of this document. Such variables
    should be named so that it is unambiguous that they are for the module
    system.
  - Don't pollute the environment. Variables should be cleaned up at the end of
    macros and functions should use variable names that don't conflict with the
    caller environment (usually by prefixing with `_function_name_` or the
    like).
  - Relocatable installs. Install trees should not bake-in paths from the build
    tree or build machine (at least by default). This makes it easier to create
    packages from install trees instead of having to run a post-processing step
    over it before it may be used for distributable packages.

[BUILD_SHARED_LIBS]: https://cmake.org/cmake/help/latest/variable/BUILD_SHARED_LIBS.html
[GNUInstallDirs]: https://cmake.org/cmake/help/latest/module/GNUInstallDirs.html
[cmake-CMAKE_PROJECT_NAME]: https://cmake.org/cmake/help/latest/variable/CMAKE_PROJECT_NAME.html
[cmake-project]: https://cmake.org/cmake/help/latest/command/project.html

# Build process

Building modules involves two phases. The first phase is called "scanning" and
involves collecting all the information necessary for the second phase,
"building". Scanning uses the [vtk_module_scan][] function to search the
[vtk.module][] files for metadata, gathers the set of modules to build and
returns them to the caller. That list of modules is eventually passed to
[vtk_module_build][] which sorts the modules for their build order and then
builds each module in turn. This separation allows for scanning and building
modules in different groups. For example, the main set of modules may be scanned
to determine which of some internal set of modules are required by those which
is then scanned separately with different options.

Scanning should occur from the leaf-most module set and work its way inward to
the lower levels. This is done so that modules in the lower level that are
required higher up can be enabled gracefully. Builds should start at the lower
level and move up the tree so that targets required by the higher groups exist
when they are built.

[vtk_module_scan]: @ref vtk_module_scan
[vtk.module]: @ref module-parse-module
[vtk_module_build]: @ref vtk_module_build

# Modules

Modules are described by [vtk.module][] files. These files are "scanned" using
the [vtk_module_scan][] function. They provide all the information necessary for
the module system to:

  - provide cache variables for selecting the module (e.g.,
    `VTK_MODULE_ENABLE_ModuleName`);
  - construct the dependency tree to automatically enable or disable modules
    based on whether it is built or not;
  - provide module-level metadata (such as exclusion from any wrapping and
    marking modules as third party)

The [vtk.module][] files are read and "parsed", but not executed directly. This
ensures that the module files do not contain any procedural CMake code. The
files may contain comments starting with `#` like CMake code. They may either
be passed manually to [vtk_module_scan][] or discovered by using the
[vtk_module_find_modules][] convenience function.

The most important (and only required) parameter is the `NAME` of a module.
This is used as the target name in CMake and is how the module's target should
be referred to in all CMake code, inside the build and from the
[`find_package`][cmake-find_package] which provides the module. To change the
name of the compiled artifact (library or executable), the `LIBRARY_NAME`
argument may be used.

It is highly recommended to provide a `DESCRIPTION` for the module. This is
added to the documentation for the cache variable so that the user has more than
just the module name to know what the module's purpose is.

Modules may also belong to groups which are created implicitly by adding
modules to the same-named group. Groups are listed under the `GROUPS` argument
and are checked in order for a non-default setting to use.

A module may be hidden by using the `CONDITION` argument. The values passed to
this field is added into a CMake `if` statement and checked for validity (all
quoting is passed along verbatim). If the condition evaluates to `FALSE`, the
module is treated as if it did not exist at all.

[vtk_module_find_modules]: @ref vtk_module_find_modules
[cmake-find_package]: https://cmake.org/cmake/help/latest/command/find_package.html

## Module metadata

A number of pieces of metadata are considered important enough to indicate them
at the module level. These are used for managing slightly different workflows
for modules which have these properties.

  - `EXCLUDE_WRAP`: This marks the module with a flag that all language wrapping
    facilities should use to know that this module is not meant for wrapping in
    any language. Usually this is for modules containing user interface classes,
    low-level functionality, or logic that is language specific.
  - `IMPLEMENTABLE` and `IMPLEMENTS`: These are used by the
    [autoinit](#autoinit) functionality to trigger the static factory
    registration calls. A module which is listed under an `IMPLEMENTS` list must
    be marked as `IMPLEMENTABLE` itself.
  - `THIRD_PARTY`: Indicates that the module represents a third party
    dependency. It may be internal or external to the source tree, but may be
    used as an additional configuration point if necessary. These modules are
    implicitly `EXCLUDE_WRAP`, not `IMPLEMENTABLE` and do not `IMPLEMENTS` any
    module.

## Enabling modules for build

Modules are enabled in a number of ways. These ways allow for project control
and user control of which modules should be built or not. There are 4 states for
controlling a module's [enable status][enable-status] as well as a `DEFAULT`
setting which is used to allow for other mechanisms to select the enable status:

  - `YES`: The module must be built.
  - `NO`: The module must not be built. If a `YES` module has a `NO` module in
    its dependency tree, an error is raised.
  - `WANT`: The module should be built. It will not be built, however, if it
    depends on a `NO` module.
  - `DONT_WANT`: The module doesn't need to be built. It will be built if a
    `YES` or `WANT` module depends on it.
  - `DEFAULT`: Look at other metadata to determine the status.

The first check for modules are via the `REQUEST_MODULES` and `REJECT_MODULES`
arguments to the `vtk_module_scan` function. Modules passed to
`REQUEST_MODULES` are treated as if they use `YES` and `REJECT_MODULES` as if
they use `NO`. A module may not be passed to both arguments. Modules selected
in this way do not have CMake cache variables exposed for them (since it is
assumed they are selected via some other mechanism outside the module system).

The next selector is the `VTK_MODULE_ENABLE_` variable for the module. This is
added to the cache and defaults to `DEFAULT`. Assuming `HIDE_MODULES_FROM_CACHE`
is not set to `ON`, this setting is exposed in the cache and allows users to
change the status of modules not handled via the `REQUEST_MODULES` and
`REJECT_MODULES` mechanism.

If a module is still selected as `DEFAULT`, the list of `GROUPS` it is a member
of is used. In order, each group is looked at for a non-`DEFAULT` value. If so,
its value is used for the module. Groups also default to using `DEFAULT` for
their setting, but a project may set the `_vtk_module_group_default_${group}`
variable to change this default value.

After all of the above logic, if a module is still marked as `DEFAULT`, the
`WANT_BY_DEFAULT` argument to [vtk_module_scan][] is used to determine whether
it is treated as a `WANT` or `DONT_WANT` request.

Now that all modules have a non-`DEFAULT` enable setting, the set of modules and
kits that are available may be determined by traversing the dependency tree of
the modules.

[enable-status]: @ref module-enable-status

## Dependencies

Modules have three types of dependencies:

  - `DEPENDS`: These are dependencies which must be available and are
    transitively provided to modules depending on this module. The API of the
    module may be affected by changes in these modules. This includes, but is
    not limited to, classes in this module inherit or expose classes from the
    dependent modules.
  - `PRIVATE_DEPENDS`: Dependencies which are only used in the implementation
    details of the module. The API of the module is not affected by changes in
    these modules.
  - `OPTIONAL_DEPENDS`: Dependencies which will be used if available, but the
    implementation can cope with their absence. These are always treated as
    `PRIVATE_DEPENDS` if they are available.

Modules which are listed in `DEPENDS` or `PRIVATE_DEPENDS` are always available
to the module and can be assumed to exist if the module is being built. Modules
listed in `OPTIONAL_DEPENDS` cannot be assumed to exist. In CMake code, a
`TARGET optional_depend` condition may be used to detect whether it is available
or not. The module system will add a `VTK_MODULE_ENABLE_${module}` compilation
definition set to either `0` or `1` if it is available for use in the module's
code. This flag is made preprocessor-safe by replacing any `::` in the module
name with `_`. So an optional dependency on `Namespace::Target` will use a flag
named `VTK_MODULE_ENABLE_Namespace_Target`.

At this stage, the dependency tree for all scanned modules is traversed, marking
dependencies of `YES` modules as those that should be built, marking modules
depending on `NO` modules as not to be built (and triggering an error if a
conflict is found). Any `WANT` modules that have not been found in the trees of
`YES` or `NO` modules are then enabled with their dependencies.

## Testing

There is some support for testing in the module system, but it is not as
comprehensive as the build side. This is because testing infrastructure and
strategies vary wildly between projects. Rather than trying to handle the
minimum baseline of any plausible testing infrastructure or framework, the
module system merely handles dependency management for testing and entering a
subdirectory with the tests.

Modules may have `TEST_DEPENDS` and `TEST_OPTIONAL_DEPENDS` lists provided as
well. These modules are required or optionally used by the testing code for the
module.

When scanning, the `ENABLE_TESTS` argument may be set to `ON`, `OFF`, `WANT`
(the default), or `DEFAULT`. Modules which appear in `TEST_DEPENDS` for the
module are affected by this setting.

  - `ON`: Modules required for testing are treated as required. Tests will be
    enabled.
  - `OFF`: Tests will not be enabled.
  - `WANT`: If possible, `TEST_DEPENDS` modules will also be enabled if they are
    not disabled in some other way.
  - `DEFAULT`: Check when tests are checked whether all of `TEST_DEPENDS` are
    available. If they are, enable testing for the module, otherwise skip it.

The only guarantee for testing provided is that all modules in the
`TEST_DEPENDS` will be available before the testing is added and
`TEST_OPTIONAL_DEPENDS` are available if they'd be available at all (i.e., they
won't be made available later).

Modules may also have `TEST_LABELS` set to ease labeling all tests for the
module. The module system itself does nothing with this other than set a global
property with the value. It is up to any test infrastructure used within the
module's CMake code to make use of the value.

The tests for a module are expected to live in a subdirectory of the module code
itself. The name of this directory is given by the `TEST_DIRECTORY_NAME`
argument to the [vtk_module_build][] function. If the directory is available and
the module's testing is enabled, the module system will
[`add_subdirectory`][cmake-add_subdirectory] this directory at the appropriate
time. This is decoupled so that testing code can depend on modules that depend
on the module that is being tested and the same `TARGET ${dependency}` check can
be used for optional module dependencies.

[cmake-add_subdirectory]: https://cmake.org/cmake/help/latest/command/add_subdirectory.html

# Building modules

After scanning is complete, [vtk_module_scan][] returns a list of modules and
kits to build in the variables given by the `PROVIDES_MODULES` and
`PROVIDES_KITS` arguments to it. It also provides lists of modules that were
found during scanning that were not scanned by that call. These are given back
in the variables passed to the `UNRECOGNIZED_MODULES` and `REQUIRES_MODULES`
variables.

The `UNRECOGNIZED_MODULES` list contains modules passed to `REQUIRES_MODULES`
and `REJECT_MODULES` that were not found during the scan. This typically
indicates that the values passed to those arguments were not constructed
properly. However, it may also mean that they should be passed on to further
scans if they may be found elsewhere. Callers should handle the variable as
necessary for their use case.

The `REQUIRES_MODULES` are modules that were named as dependencies of the
scanned modules and need to be provided in some way before building the provided
modules (the build step will require that they exist when it tries to build the
modules which required them). These can be passed on to future
`REQUIRES_MODULES` arguments in future scans or used to error out depending on
the use case of the caller.

When using [vtk_module_build][], the `PROVIDES_MODULES` and `PROVIDES_KITS` from
a single scan should be passed together. Multiple scans may be built together as
well if they all use the same build parameters as each other.

## Build-time parameters

The [vtk_module_build][] function is where the decision to build with or without
kits is decided through the `BUILD_WITH_KITS` option. Only if this is set will
kits be built for this set of modules.

The decision to default third party modules to using an external or internal
copy (where such a decision is possible) is done using the `USE_EXTERNAL`
argument.

Where build artifacts end up in the build tree are left to CMake's typical
variables for controlling these locations:

  - [`CMAKE_ARCHIVE_OUTPUT_DIRECTORY`][cmake-CMAKE_ARCHIVE_OUTPUT_DIRECTORY]
  - [`CMAKE_LIBRARY_OUTPUT_DIRECTORY`][cmake-CMAKE_LIBRARY_OUTPUT_DIRECTORY]
  - [`CMAKE_RUNTIME_OUTPUT_DIRECTORY`][cmake-CMAKE_RUNTIME_OUTPUT_DIRECTORY]

The defaults for these place outputs into the binary directory where the targets
were added. The module system will set these to be sensible for itself if they
are not already set, but it is recommended to set these at the top-level so that
targets not built under [vtk_module_build][] also end up at a sensible location.

[cmake-CMAKE_ARCHIVE_OUTPUT_DIRECTORY]: https://cmake.org/cmake/help/latest/variable/CMAKE_ARCHIVE_OUTPUT_DIRECTORY.html
[cmake-CMAKE_LIBRARY_OUTPUT_DIRECTORY]: https://cmake.org/cmake/help/latest/variable/CMAKE_LIBRARY_OUTPUT_DIRECTORY.html
[cmake-CMAKE_RUNTIME_OUTPUT_DIRECTORY]: https://cmake.org/cmake/help/latest/variable/CMAKE_RUNTIME_OUTPUT_DIRECTORY.html

## Library parameters

When building libraries, it is sometimes useful to have top-level control of
library metadata. For example, VTK suffixes its library filenames with a version
number. The variables that control this include:

  - `LIBRARY_NAME_SUFFIX`: If non-empty, all libraries and executable names will
    be suffixed with this value prefixed with a hyphen (e.g., a suffix of `foo`
    will make `Namespace::Target`'s library be named `Target-foo` or, if the
    module sets its `LIBRARY_NAME` to `nsTarget`, `nsTarget-foo`).
  - `VERSION`: Controls the [`VERSION`][cmake-VERSION] property for all library
    modules.
  - `SOVERSION`: Controls the [`SOVERSION`][cmake-SOVERSION] property for all
    library modules.

[cmake-VERSION]: https://cmake.org/cmake/help/latest/prop_tgt/VERSION.html
[cmake-SOVERSION]: https://cmake.org/cmake/help/latest/prop_tgt/SOVERSION.html

## Installation support

[vtk_module_build][] also offers arguments to aid in installing module
artifacts. These include destinations for pieces that are installed, CMake
packaging controls, and components to use for the installations.

A number of destinations control arguments are provided:

  - `ARCHIVE_DESTINATION`
  - `HEADERS_DESTINATION`
  - `LIBRARY_DESTINATION`
  - `RUNTIME_DESTINATION`
  - `CMAKE_DESTINATION`
  - `LICENSE_DESTINATION`
  - `HIERARCHY_DESTINATION`

See the API documentation for default values for each which are based on
[`GNUInstallDirs`][GNUInstallDirs] variables. Note that all installation
destinations are expected to be relative paths. This is because the conveniences
provided by the module system are all assumed to be installed to a single prefix
([`CMAKE_INSTALL_PREFIX`][cmake-CMAKE_INSTALL_PREFIX]) and placed underneath it.

Suppression of header installation is provided via the `INSTALL_HEADERS`
argument to [vtk_module_build][]. Setting this to `OFF` will suppress the
installation of:

  - headers
  - CMake package files
  - hierarchy files (since their use requires headers)

Basically, suppression of headers means that SDK components for the built
modules are not available in the install tree.

Components for the installation are provided via the `HEADERS_COMPONENT` and
`TARGETS_COMPONENT` arguments. The former is used for SDK bits and the latter
for runtime bits (libraries, executables, etc.).

For CMake package installation, the `PACKAGE` and `INSTALL_EXPORT` arguments are
available. The former controls the names used by the CMake files created by the
module system while the former is the export set to use for the member modules
when creating those CMake files. Non-module targets may also exist in this
export set when [vtk_module_build][] is called, but the export set is considered
"closed" afterwards since it has already been exported (if `INSTALL_HEADERS` is
true).

[cmake-CMAKE_INSTALL_PREFIX]: https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html

## Test data information

The directory that is looked for in each module is specified by using the
`TEST_DIRECTORY_NAME` argument. If it is set to the value of `NONE`, no testing
directories will be searched for. It defaults to `Testing` due to VTK's
conventions.

The module system, due to VTK's usage of it, has convenience parameters for
controlling the [`ExternalData`][ExternalData] module that is available to
testing infrastructure. These include:

  - `TEST_DATA_TARGET`: The data target to use for tests.
  - `TEST_INPUT_DATA_DIRECTORY`: Where `ExternalData` should look for data
    files.
  - `TEST_OUTPUT_DATA_DIRECTORY`: Where `ExternalData` should place the
    downloaded data files.
  - `TEST_OUTPUT_DIRECTORY`: Where tests should place output files.

Each is provided in the testing subdirectory as `_vtk_build_${name}`, so the
`TEST_DATA_TARGET` argument is available as `_vtk_build_TEST_DATA_TARGET`.

[ExternalData]: https://cmake.org/cmake/help/latest/module/ExternalData.html

# Building a module

Building a module is basically the same as a normal CMake library or executable,
but is wrapped to use arguments to facilitate wrapping, exporting, and
installation of the tools as well.

There are two main functions provided for this:

  - [vtk_module_add_module][]
  - [vtk_module_add_executable][]

The former creates a library for the module being built while the latter can
create an executable for the module itself or create utility executable
associated with the module. The module system requires that the `CMakeLists.txt`
for a module create a target with the name of the module. In the case of
`INTERFACE` modules, it suffices to create the module manually in many cases.

## Libraries

Most modules end up being libraries that can be linked against by other
libraries. Due to cross-platform support generally being a good thing, the
`EXPORT_MACRO_PREFIX` argument is provided to specify the prefix for macro names
to be used by [`GenerateExportHeader`][GenerateExportHeader]. By default, the
`LIBRARY_NAME` for the module is transformed to uppercase to make the prefix.

Some modules may need to add additional information to the library name that
will be used that is not statically know and depends on other environmental
settings. The `LIBRARY_NAME_SUFFIX` may be specified to add an additional suffix
to the `LIBRARY_NAME` for the module. The [vtk_module_build][]
`LIBRARY_NAME_SUFFIX` argument value will be appended to this name as well.

Normally, libraries are built according to the
[`BUILD_SHARED_LIBS`][BUILD_SHARED_LIBS] variable, however, some modules may
need to be built statically all the time. The `FORCE_STATIC` parameter exists
for this purpose. This is generally only necessary if the module is in some
other must-be-static library's dependency tree (which may happen for a number of
reasons). It is not an escape hatch for general usage; it is there because use
cases which only support static libraries (even in a shared build) exist.

If a library module is part of a kit and it is being built via the
[vtk_module_build][] `BUILD_WITH_KITS` argument, it will be built as an
[`OBJECT`][cmake-OBJECT] library and the kit machinery in [vtk_module_build][]
will create the resulting kit library artifact.

Header-only modules must pass `HEADER_ONLY` to create an `INTERFACE` library
instead of expecting a linkable artifact.

@note `HEADER_ONLY` modules which are part of kits is currently untested. This
should be supported, but might not work at the moment.

[cmake-OBJECT]: https://cmake.org/cmake/help/latest/command/add_library.html
[vtk_module_add_module]: @ref vtk_module_add_module
[vtk_module_add_executable]: @ref vtk_module_add_executable
[GenerateExportHeader]: https://cmake.org/cmake/help/latest/module/GenerateExportHeader.html

### Source listing

Instead of using CMake's "all sources in a single list" pattern for
`add_library`, [vtk_module_add_module][] classifies its source files explicitly:

  - `SOURCES`
  - `HEADERS`
  - `TEMPLATES`

The `HEADERS` and `TEMPLATES` are installed into the `HEADERS_DESTINATION`
specified to [vtk_module_build][] and may be added to a subdirectory of this
destination by using the `HEADERS_SUBDIR` argument. Note that the structure of
the header paths passed is ignored. If more structure is required from the
installed header layout, [vtk_module_install_headers][] should be used.

Files passed via `HEADERS` are treated as the API interface to the code of the
module and are added to properties so that [language wrappers](#wrapping) can
discover the API of the module.

@note Only headers passed via `HEADERS` are eligible for wrapping; those
installed via [vtk_module_install_headers][] are not. This is a known limitation
at the moment.

There are also private variations for `HEADERS` and `TEMPLATES` named
`PRIVATE_HEADERS` and `PRIVATE_TEMPLATES` respectively. These are never
installed nor exposed to wrapping mechanisms.

There are also a couple of convenience parameters that use VTK's file naming
conventions to ease usage. These include:

  - `CLASSES`: For each value `<class>`, adds `<class>.cxx` to `SOURCES` and
    `<class>.h` to `HEADERS`.
  - `TEMPLATE_CLASSES`: For each value `<class>`, adds `<class>.txx` to
    `TEMPLATES` and `<class>.h` to `HEADERS`.
  - `PRIVATE_CLASSES`: For each value `<class>`, adds `<class>.cxx` to `SOURCES`
    and `<class>.h` to `PRIVATE_HEADERS`.
  - `PRIVATE_TEMPLATE_CLASSES`: For each value `<class>`, adds `<class>.txx` to
    `PRIVATE_TEMPLATES` and `<class>.h` to `PRIVATE_HEADERS`.

[vtk_module_install_headers]: @ref vtk_module_install_headers

## Executables

Executables may be created using [vtk_module_add_executable][]. The first
argument is the name of the executable to build. Since the scanning phase does
not know what kind of target will be created for each module (and it may change
based on other configuration values), an executable module which claims it is
part of a kit raises an error since this is not possible to do.

For modules that are executables using this function, the metadata from the
module information is used to set the relevant properties. The module
dependencies are also automatically linked in the same way as a library module
would do so.

For utility executables, `NO_INSTALL` may be passed to keep it within the build
tree. It will not be available to consumers of the project. If the name of the
executable is different from the target name, `BASENAME` may be used to change
the executable's name.

## Module APIs

All of CMake's `target_` function calls have [analogues][module-as-target] for
modules. This is primarily due to the kits feature which causes the target name
created by the module system that is required to use the `target_` functions
dependent on whether the module is a member of a kit and kits are being built.
The CMake version of the function and the module API analogue (as well as
differences, if any) is:

  - [set_target_properties][cmake-set_target_properties] becomes
    [vtk_module_set_properties][]
  - [set_property(TARGET)][cmake-set_property] becomes
    [vtk_module_set_property][]
  - [get_property(TARGET)][cmake-get_property] becomes
    [vtk_module_get_property][]
  - [add_dependencies][cmake-add_dependencies] becomes [vtk_module_depend][]
  - [target_include_directories][cmake-target_include_directories] becomes
    [vtk_module_include][]
  - [target_compile_definitions][cmake-target_compile_definitions] becomes
    [vtk_module_definitions][]
  - [target_compile_options][cmake-target_compile_options] becomes
    [vtk_module_compile_options][]
  - [target_compile_features][cmake-target_compile_features] becomes
    [vtk_module_compile_features][]
  - [target_link_libraries][cmake-target_link_libraries] becomes
    [vtk_module_link][]: When kits are enabled, any `PRIVATE` links are
    forwarded to the kit itself. This necessitates making all of these targets
    globally scoped rather than locally scoped.
  - [target_link_options][cmake-target_link_options] becomes
    [vtk_module_link_options][]

[module-as-target]: @ref module-target-functions
[cmake-set_target_properties]: https://cmake.org/cmake/help/latest/command/set_target_properties.html
[cmake-set_property]: https://cmake.org/cmake/help/latest/command/set_property.html
[cmake-get_property]: https://cmake.org/cmake/help/latest/command/get_property.html
[cmake-add_dependencies]: https://cmake.org/cmake/help/latest/command/add_dependencies.html
[cmake-target_include_directories]: https://cmake.org/cmake/help/latest/command/target_include_directories.html
[cmake-target_compile_definitions]: https://cmake.org/cmake/help/latest/command/target_compile_definitions.html
[cmake-target_compile_options]: https://cmake.org/cmake/help/latest/command/target_compile_options.html
[cmake-target_compile_features]: https://cmake.org/cmake/help/latest/command/target_compile_features.html
[cmake-target_link_libraries]: https://cmake.org/cmake/help/latest/command/target_link_libraries.html
[cmake-target_link_options]: https://cmake.org/cmake/help/latest/command/target_link_options.html
[vtk_module_set_properties]: @ref vtk_module_set_properties
[vtk_module_set_property]: @ref vtk_module_set_property
[vtk_module_get_property]: @ref vtk_module_get_property
[vtk_module_depend]: @ref vtk_module_depend
[vtk_module_include]: @ref vtk_module_include
[vtk_module_definitions]: @ref vtk_module_definitions
[vtk_module_compile_options]: @ref vtk_module_compile_options
[vtk_module_compile_features]: @ref vtk_module_compile_features
[vtk_module_link]: @ref vtk_module_link
[vtk_module_link_options]: @ref vtk_module_link_options

# Packaging support

Getting installed packages to work for CMake is, unfortunately, not trivial. The
module system provides some support for helping with this, but it does place
some extra constraints on the project so that some assumptions that vastly
simplify the process can be made.

## Assumptions

The main assumption is that all modules passed to a single [vtk_module_build][]
have the same CMake namespace (the part up to and including the `::`, if any,
in a module name. For exporting dependencies, that namespace matches the
`PACKAGE` argument for [vtk_module_build][]. These are done so that the
generated code can use
[`CMAKE_FIND_PACKAGE_NAME`][cmake-CMAKE_FIND_PACKAGE_NAME] variable can be
used to discover information about the package that is being found.

The package support also assumes that all modules may be queried using
`COMPONENTS` and `OPTIONAL_COMPONENTS` and that the component name for a module
corresponds to the name of a module without the namespace.

These rules basically mean that a module named `Namespace::Target` will be found
using `find_package(Namespace)`, that `COMPONENTS Target` may be passed to
ensure that that module exists, and `OPTIONAL_COMPONENTS Target` may be passed
to allow the component to not exist while not failing the main
[`find_package`][cmake-find_package] call.

[cmake-CMAKE_FIND_PACKAGE_NAME]: https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_PACKAGE_NAME.html

## Creating a full package

The module system provides no support for the top-level file that is used by
[`find_package`][cmake-find_package]. This is because this logic is highly
project-specific and hard to generalize in a useful way. Instead, files are
generated which should be included from the main file.

Here, the list of files generated are based on the `PACKAGE` argument passed to
[vtk_module_build][]:

  - `<PACKAGE>-targets.cmake`: The CMake-generated export file for the targets
    in the `INSTALL_EXPORT`.
  - `<PACKAGE>-vtk-module-properties.cmake`: Properties for the targets exported
    into the build.

The module properties file must be included after the targets file so that they
exist when it tries to add properties to the imported targets.

## External dependencies

Since the module system is heavily skewed towards using imported targets, these
targets show up by name in the [`find_package`][cmake-find_package] of the
project as well. This means that these external projects need to be found to
recreate their imported targets at that time. To this end, there is the
[vtk_module_export_find_packages][] function. This function writes a file named
according to its `FILE_NAME` argument and place it in the build and install
trees according to its `CMAKE_DESTINATION` argument.

This file will be populated with logic to determine whether third party packages
found using [vtk_module_find_package][] are required during the
[`find_package`][cmake-find_package] of the package or not. It will forward
`REQUIRED` and `QUIET` parameters to other [`find_package`][cmake-find_package]
calls as necessary based on the `REQUIRED` and `QUIET` flags for the package
and whether that call is involved in a non-optional `COMPONENT` (a
component-less [`find_package`][cmake-find_package] call is assumed to mean
"all components").

This file should be included after the `<PACKAGE>-vtk-module-properties.cmake`
file generated by the [vtk_module_build][] call so that it can use the module
dependency information set via that file.

After this file is included, for each component that it checks, it will set
`${CMAKE_FIND_PACKAGE_NAME}_<component>_FOUND` to 0 if it is not valid and
append a reason to `${CMAKE_FIND_PACKAGE_NAME}_<component>_NOT_FOUND_MESSAGE`
so that the package can collate the reason why things are not available.

[vtk_module_export_find_packages]: @ref vtk_module_export_find_packages
[vtk_module_find_package]: @ref vtk_module_find_package

## Setting the `_FOUND` variable

The module system does not currently help in determining the top-level
`${CMAKE_FIND_PACKAGE_NAME}_FOUND` variable based on the results of the
components that were requested and the status of dependent packages. This may be
provided at some point, but there has not currently been enough experience to
determine what patterns are available for factoring it out as a utility
function.

The general pattern should be to go through the list of components requested,
determine whether targets for those components exist. Then for each found
component, use the module dependency information to ensure that all targets in
the dependency trees are found (propagating not-found statuses through the
dependency tree). The `${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE` should be
built up based on the reasons the [`find_package`][cmake-find_package] call did
not work based on these discoveries.

This is the process for modules in a package, but packages may contain
non-module components, and it is hard for the module system to provide support
for them, so they are not attempted. See the CMake documentation for more
details about [creating a package configuration][cmake-create-package-config].

[cmake-create-package-config]: https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html#creating-a-package-configuration-file

# Advanced topics

There are a number of advanced features provided by the module system that are
not normally required in a simple project.

## Kits

Kits are described in [vtk.kit][] files which act much like [vtk.module][]
files. However, they only have `NAME`, `LIBRARY_NAME`, and `DESCRIPTION` fields.
These all act just like they do in the `vtk.module` context. These files may
either be passed manually to [vtk_module_scan][] or discovered by using the
[vtk_module_find_kits][] convenience function.

Before a module may be a member of a kit, a [vtk.kit][] must declare it and be
scanned at the same time. This means that kits may only contain modules that are
scanned with them and cannot be extended later nor may kits be made of modules
that they do not know about.

[vtk.kit]: @ref module-parse-kit
[vtk_module_find_kits]: @ref vtk_module_find_kits

### Requirements

In order to actually use kits, CMake 3.12 is necessary in order to do the
[`OBJECT`][cmake-OBJECT] library manipulations done behind the scenes to make it
Just Work. 3.8 is still the minimum version for using a project that is built
with kits however. This is only checked when kits are actually in use, so
projects requiring older CMake versions as their minimum version may still
provide kits so that users with newer CMake versions can use them.

Kits create a single library on disk, but the usage requirements of the modules
should still be the same (except for that which is inherently required to be
different by combining libraries). So include directories, compile definitions,
and other usage requirements should not leak from other modules that are members
of the same kit.

<a name="autoinit"></a>
## Autoinit

The module system supports a mechanism for triggering static code construction
for modules which require it. This cannot be done through normal CMake usage
requirements because the requirements are intersectional. For example, a module
`F` having a factory where module `I` provides an implementation for it means
that a target linking to both `F` and `I` needs to ensure that `I` registers its
implementation to the factory code. There is no such support in CMake and due to
the complexities and code generation involved with this support, it is unlikely
to exist.

Code which uses modules may call the [vtk_module_autoinit][] function to use
this functionality. The list of modules passed to the function are used to
compute the defines necessary to trigger the registration to factories when
necessary.

For details on the implementation of the autoinit system, please see
[the relevant section][autoinit] in the API documentation.

[vtk_module_autoinit]: @ref vtk_module_autoinit
[autoinit]: @ref module-autoinit

<a name="wrapping"></a>
## Wrapping

VTK comes with support for wrapping its classes into other languages.
Currently, VTK supports wrapping its classes for use in the Python and Java
languages. In order to wrap a set of modules for a language, a separate
function is used for each language.

All languages read the headers of classes with a `__VTK_WRAP__` preprocessor
definition defined. This may be used to hide methods or other details from the
wrapping code if wanted.

### Python

For Python, the [vtk_module_wrap_python][] function must be used. This function
takes a list of modules in its `MODULES` argument and creates Python modules
for use under the `PYTHON_PACKAGE` package. No `__init__.py` for this package
is created automatically and must be provided in some other way.

A target named by the `TARGET` argument is created and installed. This target
may be linked to in order to be able to import static Python modules. In this
case, a header and function named according to the basename of `TARGET` (e.g.,
`VTK::PythonWrapped` has a basename of `PythonWrapped`) must be used. The
header is named `<TARGET_BASENAME>.h` and the function which adds the wrapped
modules to the static import table is `<void TARGET_BASENAME>_load()`. This
function is also created in shared builds, but does nothing so that it may
always be called in static or shared builds.

The modules will be installed under the `MODULE_DESTINATION` given to the
function into the `PYTHON_PACKAGE` directory needed for it. The
[vtk_module_python_default_destination][] function is used to determine a
default if one is not passed.

The Python wrappers define a `__VTK_WRAP_PYTHON__` preprocessor definition when
reading code which may be used to hide methods or other details from the Python
wrapping code.

[vtk_module_wrap_python]: @ref vtk_module_wrap_python
[vtk_module_python_default_destination]: @ref vtk_module_python_default_destination

### Java

For Java, the [vtk_module_wrap_java][] function must be used. This function
creates Java sources for classes in the modules passed in its `MODULES`
argument. The sources are written to a `JAVA_OUTPUT` directory. These then can
be compiled by CMake normally.

For this purpose, there are `<MODULE>Java` targets which contain a
`_vtk_module_java_files` properties containing a list of `.java` sources
generated for the given module. There is also a `<MODULE>Java-java-sources`
target which may be depended upon if just the source generation needs to used
in an [`add_dependencies`][cmake-add_dependencies] call.

The Java wrappers define a `__VTK_WRAP_JAVA__` preprocessor definition when
reading code which may be used to hide methods or other details from the Java
wrapping code.

[vtk_module_wrap_java]: @ref vtk_module_wrap_java

### Hierarchy files

Hierarchy files are used by the language wrapper tools to know the class
inheritance for classes within a module. Each module has a hierarchy file
associated with it. The path to a module's hierarchy file is stored in its
`hierarchy` module property.

## Third party

The module system has support for representing third party modules in its
build. These may be built as part of the project or represented using other
mechanisms (usually [`find_package`][cmake-find_package] and a set of imported
targets from it).

The primary API is [vtk_module_third_party][] which creates a
`VTK_MODULE_USE_EXTERNAL_Namespace_Target` option for the module to switch
between an internal and external source for the third party code. This value
defaults to the setting of the `USE_EXTERNAL` argument for the calling
[vtk_module_build][] function. Arguments passed under the `INTERNAL` and
`EXTERNAL` arguments to this command are then passed on to
[vtk_module_third_party_internal][] or [vtk_module_third_party_external][],
respectively, depending on the `VTK_MODULE_USE_EXTERNAL_Namespace_Target`
option.

Note that third party modules (marked as such by adding the `THIRD_PARTY`
keyword to a `vtk.module` file) may not be part of a kit, be wrapped, or
participate in autoinit.

[vtk_module_third_party]: @ref vtk_module_third_party
[vtk_module_third_party_internal]: @ref vtk_module_third_party_internal
[vtk_module_third_party_external]: @ref vtk_module_third_party_external

### External third party modules

External modules are found using CMake's [`find_package`][cmake-find_package]
mechanism. In addition to the arguments supported by
[vtk_module_find_package][] (except `PRIVATE`), information about the found
package is used to construct a module target which represents the third party
package. The preferred mechanism is to give a list of imported targets to the
`LIBRARIES` argument. These will be added to the `INTERFACE` of the module and
provide the third party package for use within the module system.

If imported targets are not available (they really should be created if not),
variable names may be passed to `INCLUDE_DIRS`, `LIBRARIES`, and `DEFINITIONS`
to create the module interface.

In addition, any variables which should be forwarded from the package to the
rest of the build may be specified using the `USE_VARIABLES` argument.

The `STANDARD_INCLUDE_DIRS` argument creates an include interface for the
module target which includes the "standard" module include directories to.
Basically, the source and binary directories of the module.

### Internal third party modules

Internal modules are those that may be built as part of the build. These should
ideally specify a set of `LICENSE_FILES` indicating the license status of the
third party code. These files will be installed along with the third party
package to aid in any licensing requirements of the code. It is also
recommended to set the `VERSION` argument so that it is known what version of
the code is provided at a glance.

By default, the `LIBRARY_NAME` of the module is used as the name of the
subdirectory to include, but this may be changed by using the `SUBDIRECTORY`
argument.

Header-only third party modules may be indicated by using the `HEADER_ONLY`
argument. Modules which represent multiple libraries at once from a project may
use the `INTERFACE` argument.

The `STANDARD_INCLUDE_DIRS` argument creates an include interface for the
module target which includes the "standard" module include directories to.
Basically, the source and binary directories of the module. A subdirectory may
be used by setting the `HEADERS_SUBDIR` option. It is implied for
`HEADERS_ONLY` third party modules.

After the subdirectory is added a target with the module's name must exist.
However, a target is automatically created if it is `HEADERS_ONLY`.

#### Properly shipping internal third party code

There are many things that really should be done to ship internal third party
code (also known as vendoring) properly. The issue is mainly that the internal
code may conflict with other code bringing in another copy of the same package
into a process. Most platforms do not behave well in this situation.

In order to avoid conflicts at every level possible, a process called "name
mangling" should be performed. A non-exhaustive list of name manglings that
must be done to fully handle this case includes:

  - moving headers to a subdirectory (to avoid compilations from finding
    incompatible headers);
  - changing the library name (to avoid DLL lookups from finding incompatible
    copies); and
  - mangling symbols (to avoid symbol lookup from confusing two copies in the
    same process).

Some projects may need further work like editing CMake APIs or the like to be
mangled as well.

Moving headers and changing library names is fairly straightforward by editing
CMake code. Mangling symbols usually involves creating a header which has a
`#define` for each public symbol to change its name at runtime to be distinct
from another copy that may end up existing in the same process from another
project.

Typically, a header needs to be created at the module level which hides the
differences between third party code which may or may not be provided by an
external package. In this case, it is recommended that code using the third
party module use unmangled names and let the module interface and mangling
headers handle the mangling at that level.

## Debugging

The module system can output debugging information about its inner workings by
using the `_vtk_module_log` variable. This variable is a list of "domains" to
log about, or the special `ALL` value causes all domains to log output. The
following domains are used in the internals of the module system:

  - `kit`: discovery and membership of kits
  - `module`: discovery and `CONDITION` results of modules
  - `enable`: resolution of the enable status of modules
  - `provide`: determination of module provision
  - `building`: when building a module occurs
  - `testing`: missing test dependencies

It is encouraged that projects expose user-friendly flags to control logging
rather than exposing `_vtk_module_log` directly.

## Control variables

These variables do not follow the API convention and are used if set:

  - `_vtk_module_warnings`: If enabled, "strict" warnings are generated. These
    are not strictly problems, but may be used as linting for improving usage of
    the module system.
  - `_vtk_module_log`: A list of "domains" to output debugging information.
  - `_vtk_module_group_default_${group}`: used to set a non-`DEFAULT` default
    for group settings.

Some mechanisms use global properties instead:

  - `_vtk_module_autoinit_include`: The file that needs to be included in order
    to make the `VTK_MODULE_AUTOINIT` symbol available for use in the
    [autoinit][] support.
