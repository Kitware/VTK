function (_vtk_package_append_variables)
  set(_vtk_package_variables)
  foreach (var IN LISTS ARGN)
    if (NOT ${var})
      continue ()
    endif ()

    string(APPEND _vtk_package_variables
      "if (NOT DEFINED \"${var}\")
  set(\"${var}\" \"${${var}}\")
  list(APPEND _vtk_find_package_variables \"${var}\")
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

set(vtk_find_package_code)
foreach (_vtk_package IN LISTS _vtk_packages)
  _vtk_package_append_variables(
    # Standard CMake `find_package` mechanisms.
    "${package}_DIR"
    "${package}_ROOT"

    # Per-package custom variables.
    ${${package}_find_package_vars})
endforeach ()

file(GENERATE
  OUTPUT  "${vtk_cmake_build_dir}/vtk-find-package-helpers.cmake"
  CONTENT "${vtk_find_package_code}")
