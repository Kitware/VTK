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
