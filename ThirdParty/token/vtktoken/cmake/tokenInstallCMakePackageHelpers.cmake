function (_token_package_append_variables)
  set(_token_package_variables)
  foreach (var IN LISTS ARGN)
    if (NOT DEFINED "${var}")
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

    string(APPEND _token_package_variables
      # Only set the variable as a helper entry if there isn't already a value for it.
      "if (NOT DEFINED \"${var}\" OR NOT ${var})
  set(\"${var}\" \"${${var}}\" CACHE ${type} \"Third-party helper setting from \${CMAKE_FIND_PACKAGE_NAME}\")
endif ()
")
  endforeach ()

  set(token_find_package_code
    "${token_find_package_code}${_token_package_variables}"
    PARENT_SCOPE)
endfunction ()

set(_token_packages
  nlohmann_json
)

set(token_find_package_code)
foreach (_token_package IN LISTS _token_packages)
  _token_package_append_variables(
    # Standard CMake `find_package` mechanisms.
    "${_token_package}_DIR"
    "${_token_package}_ROOT"

    # Per-package custom variables.
    ${${_token_package}_find_package_vars})
endforeach ()

file(GENERATE
  OUTPUT  "${token_cmake_build_dir}/token-find-package-helpers.cmake"
  CONTENT "${token_find_package_code}")
