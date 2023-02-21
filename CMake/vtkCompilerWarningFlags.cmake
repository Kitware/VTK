# This module requires CMake 3.19 features (the `CheckCompilerFlag`
# module). Just skip it for older CMake versions.
if (CMAKE_VERSION VERSION_LESS "3.19")
  return ()
endif ()

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

check_compiler_flag(C "-Weverything" vtk_have_compiler_flag_Weverything)
include(CMakeDependentOption)
cmake_dependent_option(VTK_ENABLE_EXTRA_BUILD_WARNINGS_EVERYTHING "Enable *all* warnings (except known problems)" OFF
  "VTK_ENABLE_EXTRA_BUILD_WARNINGS;vtk_have_compiler_flag_Weverything" OFF)
mark_as_advanced(VTK_ENABLE_EXTRA_BUILD_WARNINGS_EVERYTHING)

# MSVC
# Disable flags about `dll-interface` of inherited classes.
vtk_add_flag(-wd4251 CXX)
# Enable C++ stack unwinding and that C functions never throw C++
# exceptions.
vtk_add_flag(-EHsc CXX)

if (VTK_ENABLE_EXTRA_BUILD_WARNINGS_EVERYTHING)
  set(langs C CXX)
  vtk_add_flag(-Weverything ${langs})

  # Instead of enabling warnings, this mode *disables* warnings.
  vtk_add_flag(-Wno-cast-align ${langs})
  vtk_add_flag(-Wno-cast-function-type-strict ${langs})
  vtk_add_flag(-Wno-cast-qual ${langs})
  vtk_add_flag(-Wno-conversion ${langs})
  vtk_add_flag(-Wno-covered-switch-default ${langs})
  vtk_add_flag(-Wno-declaration-after-statement ${langs})
  vtk_add_flag(-Wno-direct-ivar-access ${langs})
  vtk_add_flag(-Wno-disabled-macro-expansion ${langs})
  vtk_add_flag(-Wno-documentation ${langs})
  vtk_add_flag(-Wno-documentation-unknown-command ${langs})
  vtk_add_flag(-Wno-double-promotion ${langs})
  vtk_add_flag(-Wno-exit-time-destructors ${langs})
  vtk_add_flag(-Wno-extra-semi ${langs})
  vtk_add_flag(-Wno-extra-semi-stmt ${langs})
  vtk_add_flag(-Wno-float-equal ${langs})
  vtk_add_flag(-Wno-format-nonliteral ${langs})
  vtk_add_flag(-Wno-format-pedantic ${langs})
  vtk_add_flag(-Wno-global-constructors ${langs})
  vtk_add_flag(-Wno-long-long ${langs})
  vtk_add_flag(-Wno-missing-noreturn ${langs})
  vtk_add_flag(-Wno-missing-prototypes ${langs})
  vtk_add_flag(-Wno-missing-variable-declarations ${langs})
  vtk_add_flag(-Wno-objc-interface-ivars ${langs})
  vtk_add_flag(-Wno-padded ${langs})
  vtk_add_flag(-Wno-reserved-id-macro ${langs})
  vtk_add_flag(-Wno-shorten-64-to-32 ${langs})
  vtk_add_flag(-Wno-sign-conversion ${langs})
  vtk_add_flag(-Wno-strict-prototypes ${langs})
  vtk_add_flag(-Wno-switch-enum ${langs})
  vtk_add_flag(-Wno-undef ${langs})
  vtk_add_flag(-Wno-unused-macros ${langs})
  vtk_add_flag(-Wno-vla ${langs})
  vtk_add_flag(-Wno-vla-extension ${langs})

  set(langs CXX)
  vtk_add_flag(-Wno-c++98-compat-pedantic ${langs})
  vtk_add_flag(-Wno-inconsistent-missing-override ${langs})
  vtk_add_flag(-Wno-old-style-cast ${langs})
  vtk_add_flag(-Wno-return-std-move-in-c++11 ${langs})
  vtk_add_flag(-Wno-signed-enum-bitfield ${langs})
  vtk_add_flag(-Wno-undefined-func-template ${langs})
  vtk_add_flag(-Wno-unused-member-function ${langs})
  vtk_add_flag(-Wno-weak-template-vtables ${langs})
  vtk_add_flag(-Wno-weak-vtables ${langs})
  vtk_add_flag(-Wno-zero-as-null-pointer-constant ${langs})
  vtk_add_flag(-Wno-unsafe-buffer-usage ${langs})

  # These should be fixed at some point prior to updating VTK's standard
  # version. See #18585.
  vtk_add_flag(-Wno-deprecated-copy-dtor ${langs})
  vtk_add_flag(-Wno-deprecated-copy ${langs})
elseif (VTK_ENABLE_EXTRA_BUILD_WARNINGS)
  # C flags.
  set(langs C)

  # C++ flags.
  set(langs CXX)
  vtk_add_flag(-Winconsistent-missing-destructor-override ${langs})
  vtk_add_flag(-Wnon-virtual-dtor ${langs})
  vtk_add_flag(-Wpessimizing-move ${langs})
  vtk_add_flag(-Wrange-loop-bind-reference ${langs})
  vtk_add_flag(-Wreorder-ctor ${langs})
  vtk_add_flag(-Wunused-lambda-capture ${langs})
  vtk_add_flag(-Wunused-private-field ${langs})

  # C and C++ flags.
  set(langs C CXX)
  vtk_add_flag(-Wabsolute-value ${langs})
  vtk_add_flag(-Wsign-compare ${langs})
  vtk_add_flag(-Wunreachable-code ${langs})
  vtk_add_flag(-Wunused-but-set-variable ${langs})
  vtk_add_flag(-Wunused-function ${langs})
  vtk_add_flag(-Wunused-local-typedef ${langs})
  vtk_add_flag(-Wunused-parameter ${langs})
  vtk_add_flag(-Wunused-variable ${langs})

  # Fortran flags.
endif ()
