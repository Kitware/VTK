function (_vtk_package_append_variables)
  set(_vtk_package_variables)
  foreach (var IN LISTS ARGN)
    if (NOT ${var})
      continue ()
    endif ()

    get_property(type_is_set CACHE "${var}"
      PROPERTY TYPE SET)
    if (type_is_set)
      get_property(type CACHE "${var}"
        PROPERTY TYPE)
    else ()
      set(type UNINITIALIZED)
    endif ()

    string(APPEND _vtk_package_variables
      "if (NOT DEFINED \"${var}\" OR NOT ${var})
  set(\"${var}\" \"${${var}}\" CACHE ${type} \"Third-party helper setting from \${CMAKE_FIND_PACKAGE_NAME}\")
endif ()
")
  endforeach ()

  set(vtk_find_package_code
    "${vtk_find_package_code}${_vtk_package_variables}"
    PARENT_SCOPE)
endfunction ()

get_property(_vtk_packages GLOBAL
  PROPERTY _vtk_module_find_packages_VTK)
if (_vtk_packages)
  list(REMOVE_DUPLICATES _vtk_packages)
endif ()

# Per-package variable forwarding goes here.
set(Boost_find_package_vars
  Boost_INCLUDE_DIR)
set(OSMesa_find_package_vars
  OSMESA_INCLUDE_DIR
  OSMESA_LIBRARY)

if ("ospray" IN_LIST _vtk_packages)
  # FIXME: ospray depends on embree, but does not help finders at all.
  # https://github.com/ospray/ospray/issues/352
  list(APPEND _vtk_packages
    embree)
endif ()

set(vtk_find_package_code)
foreach (_vtk_package IN LISTS _vtk_packages)
  _vtk_package_append_variables(
    # Standard CMake `find_package` mechanisms.
    "${_vtk_package}_DIR"
    "${_vtk_package}_ROOT"

    # Per-package custom variables.
    ${${_vtk_package}_find_package_vars})
endforeach ()

file(GENERATE
  OUTPUT  "${vtk_cmake_build_dir}/vtk-find-package-helpers.cmake"
  CONTENT "${vtk_find_package_code}")
