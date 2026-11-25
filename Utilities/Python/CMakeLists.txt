#[==[.md
# Python support

This module provides a single target for using Python within VTK modules.

## Flexible Python libraries

Ideally, the libraries linked against the `vtkPython` module would be flexible
in the library actually used and it would be provided by the executable loading
the Python instead. This can be made to work with hacks currently, but the
following should be fixed first:

  - A better solution than `-undefined dynamic_lookup` for macOS. VTK has
    [an issue][VTK dynamic Python lookup issue] filed for this already.

[VTK dynamic Python lookup issue]: https://gitlab.kitware.com/vtk/vtk/-/issues/17214
#]==]

# VTK only supports a single Python version at a time, so make artifact finding
# interactive.
set(Python3_ARTIFACTS_INTERACTIVE ON)

set(vtk_python_min_version "3.4")
set(vtk_python_version_support "3.10")
if (VTK_REMOVE_LEGACY)
  set(vtk_python_min_version "${vtk_python_version_support}")
endif ()
vtk_module_find_package(
  PACKAGE Python3
  VERSION "${vtk_python_min_version}"
  COMPONENTS Interpreter Development.Module
  OPTIONAL_COMPONENTS Development.Embed
  FORWARD_VERSION_REQ MINOR)
set(vtk_python_includes "${Python3_INCLUDE_DIRS}")
set(vtk_python_target "Python3::Module")
set(vtk_python_embed_target "Python3::Python")
set(vtk_python_version "${Python3_VERSION}")
set(vtk_python_version_pair "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")

set_property(GLOBAL PROPERTY _vtk_python_soabi "${Python3_SOABI}")
set_property(GLOBAL PROPERTY _vtk_python_version_major "${Python3_VERSION_MAJOR}")
set_property(GLOBAL PROPERTY _vtk_python_version_minor "${Python3_VERSION_MINOR}")

# Check deprecated versions of Python
if (vtk_python_version VERSION_LESS vtk_python_version_support)
  message(DEPRECATION
    "Python ${vtk_python_version} support is deprecated, use Python ${vtk_python_version_support}+")
endif ()

# Export location of python module dirs in install and build tree for every vtkpython module to use
# As long as those modules depend on vtkpython, they can retrieve and use these
# When VTK_PYTHON_LIMITED_API (abi3) is ON, the top-level CMakeLists.txt has
# already chosen a version-neutral suffix (no pythonX.Y component); this fallback
# only runs when the suffix was not computed there.
if (NOT VTK_PYTHON_SITE_PACKAGES_SUFFIX)
  if (MSVC)
    set(VTK_PYTHON_SITE_PACKAGES_SUFFIX "Lib/site-packages")
  elseif (VTK_PYTHON_LIMITED_API)
    set(VTK_PYTHON_SITE_PACKAGES_SUFFIX "python${Python3_VERSION_MAJOR}/site-packages")
  else ()
    set(VTK_PYTHON_SITE_PACKAGES_SUFFIX
      "python${vtk_python_version_pair}/site-packages")
  endif ()
endif ()

if (CMAKE_CONFIGURATION_TYPES)
  # For build systems with configuration types e.g. Xcode/Visual Studio,
  # we rely on generator expressions.
  set(VTK_BUILD_PYTHON_MODULES_DIR
    "${CMAKE_BINARY_DIR}/$<CONFIG>/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}")
else ()
  set(VTK_BUILD_PYTHON_MODULES_DIR
    "${CMAKE_BINARY_DIR}/${VTK_PYTHON_SITE_PACKAGES_SUFFIX}")
endif ()

if (NOT DEFINED VTK_INSTALL_PYTHON_MODULES_DIR)
  if (WIN32 AND NOT CYGWIN)
    set(VTK_INSTALL_PYTHON_MODULES_DIR
      "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}")
  else ()
    set(VTK_INSTALL_PYTHON_MODULES_DIR
      "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}")
  endif ()
endif ()

# For static builds, since vtkPythonInterpreter cannot work off the library
# location, but instead has to use the executable location, it needs to know the
# library dir explicitly.
if (NOT BUILD_SHARED_LIBS)
  set(VTK_PYTHON_SITE_PACKAGES_SUFFIX_FIXED "${VTK_INSTALL_PYTHON_MODULES_DIR}")
else ()
  set(VTK_PYTHON_SITE_PACKAGES_SUFFIX_FIXED "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}")
endif ()

# PY_VERSION_HEX floor for the Limited API.  3.11 (0x030B0000) is the lowest
# floor that supports everything the wrappers need (the buffer protocol and
# PyType_GetName entered the Limited API in 3.11); 3.10 and earlier cannot
# express the buffer protocol used for array/NumPy interop.
set(VTK_PYTHON_LIMITED_API_VERSION "0x030B0000" CACHE STRING
  "Minimum Python version (PY_VERSION_HEX form) for the Limited API floor")
mark_as_advanced(VTK_PYTHON_LIMITED_API_VERSION)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkPythonConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPythonConfigure.h")

set(headers
  vtkPython.h
  "${CMAKE_CURRENT_BINARY_DIR}/vtkPythonConfigure.h")

vtk_module_add_module(VTK::Python
  HEADERS ${headers}
  HEADER_ONLY)

include(CMakeDependentOption)
cmake_dependent_option(VTK_PYTHON_OPTIONAL_LINK
  # Default to `ON` if it is possible.
  "Whether to link libpython from libraries or not" ON
  # We shouldn't do it for static builds and we can't do it without
  # `target_link_options`. Windows also always needs to link against libpython
  # directly.
  "NOT WIN32;BUILD_SHARED_LIBS" OFF)

add_library(PythonUsed INTERFACE)
add_library(VTK::PythonUsed ALIAS PythonUsed)
_vtk_module_install(PythonUsed)

# abi3 on Windows: link the stable python3.lib rather than the versioned
# python3XY.lib.  Once Py_LIMITED_API is defined, CPython's pyconfig.h auto-links
# python3.lib through `#pragma comment(lib, ...)`, and that import library
# forwards to python3.dll, which is present for every Python 3.x.  We therefore
# only expose the Python headers and put the import library directory on the
# linker search path; we deliberately do not link Python3::Module/Python here,
# which would otherwise pull in the version-specific import library and tie the
# module to a single python3XY.dll.  Translation units that opt out of the
# Limited API (the embedded interpreter) keep emitting the versioned pragma and
# resolve python3XY.lib from the same directory, so both coexist.
if (VTK_PYTHON_LIMITED_API AND WIN32)
  if (DEFINED Python3_LIBRARY_DIRS)
    set(_vtk_python_limited_libdir "${Python3_LIBRARY_DIRS}")
  else ()
    # XXX(cmake-3.24): Use `$<PATH:GET_PARENT_PATH,$<LOCATION:Python::Module>>`.
    list(GET Python3_LIBRARIES 0 _vtk_python_limited_lib0)
    get_filename_component(_vtk_python_limited_libdir
      "${_vtk_python_limited_lib0}" DIRECTORY)
  endif ()
  add_library(vtkPythonLimitedABI INTERFACE)
  add_library(VTK::PythonLimitedABI ALIAS vtkPythonLimitedABI)
  # XXX(cmake-3.26): Use `$<BUILD_LOCAL_INTERFACE>`.
  target_include_directories(vtkPythonLimitedABI SYSTEM INTERFACE
    "$<BUILD_INTERFACE:${Python3_INCLUDE_DIRS}>")
  target_link_directories(vtkPythonLimitedABI INTERFACE
    "$<BUILD_INTERFACE:${_vtk_python_limited_libdir}>")
  _vtk_module_install(vtkPythonLimitedABI)
  vtk_module_link(VTK::Python
    INTERFACE
      VTK::PythonLimitedABI)
# If we want optional linking and we have a real libpython, set up the forward
# linking.
elseif (VTK_PYTHON_OPTIONAL_LINK AND TARGET "${vtk_python_embed_target}")
  # It has been observed that Ubuntu's GCC toolchain defaults require flags,
  # but are not detected here due to the confluence of the flags (basically,
  # the errors only show up in an executable that links to Python-using code,
  # but does not use Python itself either). Since this is not detected, setting
  # `vtk_undefined_symbols_allowed=OFF` manually may be required when using
  # Ubuntu toolchains.
  include("${CMAKE_CURRENT_SOURCE_DIR}/vtkUndefinedSymbolsAllowed.cmake")
  set(is_exe "$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>")
  set(needs_flags "$<NOT:$<BOOL:${vtk_undefined_symbols_allowed}>>")
  set(direct_link "$<BOOL:$<TARGET_PROPERTY:INTERFACE_vtk_python_direct_link>>")
  set(should_use "$<OR:${is_exe},${direct_link}>")
  # Intel's Fortran compiler doesn't like some flags, so mask it out when using
  # the Intel Fortran Compiler (OneAPI).
  if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15") # XXX(cmake-3.15): `COMPILE_LANG_AND_ID` genex
    set(ifx_mask "$<NOT:$<COMPILE_LANG_AND_ID:Fortran,IntelLLVM>>")
  else ()
    set(ifx_mask 1)
  endif ()
  set(platform_flags
    # Apple flags.
    "$<$<PLATFORM_ID:Darwin>:-undefined;dynamic_lookup>"
    # Linux flags.
    # The linker is allowed to skip libraries not used by the end result. This
    # can exclude the `libpython` DT_NEEDED entry at the final executable if it
    # itself does not actually use any libpython symbols.
    "$<$<AND:${should_use},$<PLATFORM_ID:Linux>>:LINKER:--no-as-needed>"
    # Due to the above making the symbols actually available, we can ignore
    # unresolved symbols in the shared libraries that are being linked to.
    "$<$<AND:${should_use},$<PLATFORM_ID:Linux>,${ifx_mask}>:LINKER:--unresolved-symbols=ignore-in-shared-libs>")

  # Use the `PythonX::Module` target from FindPython.
  vtk_module_link(VTK::Python
    INTERFACE
      "${vtk_python_target}")

  # This target is intended for use by the end-result executable with a
  # `VTK::Python` link in its total library closure. This exposes the
  # `PythonX::Python` link at the executable level with the required platform
  # flags.
  target_link_libraries(PythonUsed
    INTERFACE
      "$<LINK_ONLY:$<${should_use}:${vtk_python_embed_target}>>")
  target_link_options(PythonUsed
    INTERFACE
      "$<$<AND:${should_use},${needs_flags}>:${platform_flags}>")
  _vtk_module_set_module_property(VTK::Python APPEND
    PROPERTY  "forward_link"
    VALUE     VTK::PythonUsed)
# Otherwise, if we have the libpython and aren't doing optional linking, just
# use libpython.
elseif (NOT VTK_PYTHON_OPTIONAL_LINK AND TARGET "${vtk_python_embed_target}")
  vtk_module_link(VTK::Python
    INTERFACE
      "${vtk_python_embed_target}")
# Do the optional linking support.
else ()
  vtk_module_link(VTK::Python
    INTERFACE
      "${vtk_python_target}")
endif ()
