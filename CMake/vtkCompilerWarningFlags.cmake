include(CheckCompilerFlag)

function (vtk_add_flag flag)
  foreach (lang IN LISTS ARGN)
    check_compiler_flag("${lang}" "${flag}" "vtk_have_compiler_flag-${lang}-${flag}")
    if (vtk_have_compiler_flag-${lang}-${flag})
      target_compile_options(vtkbuild
        INTERFACE
          "$<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:${lang}>:${flag}>>")
    endif ()
  endforeach ()
endfunction ()

option(VTK_ENABLE_EXTRA_BUILD_WARNINGS "Enable extra build warnings" OFF)
mark_as_advanced(VTK_ENABLE_EXTRA_BUILD_WARNINGS)

# MSVC
# Disable flags about `dll-interface` of inherited classes.
vtk_add_flag(-wd4251 CXX)
# Enable C++ stack unwinding and that C functions never throw C++
# exceptions.
vtk_add_flag(-EHsc CXX)

if (VTK_ENABLE_EXTRA_BUILD_WARNINGS)
  # C flags.

  # C++ flags.
  vtk_add_flag(-Winconsistent-missing-destructor-override CXX)

  # C and C++ flags.

  # Fortran flags.
endif ()
