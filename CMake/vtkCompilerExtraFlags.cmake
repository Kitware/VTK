if(CMAKE_COMPILER_IS_GNUCXX)

  include(CheckCXXCompilerFlag)

  # Additional warnings for GCC
  set(CMAKE_CXX_FLAGS_WARN "-Wnon-virtual-dtor -Wno-long-long -ansi -Wcast-align -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wformat-security -Woverloaded-virtual -Wshadow -Wunused-parameter -fno-check-new -fno-common -Werror=undef")

  # This flag is useful as not returning from a non-void function is an error
  # with MSVC, but it is not supported on all GCC compiler versions
  check_cxx_compiler_flag(-Werror=return-type HAVE_GCC_ERROR_RETURN_TYPE)
  if(HAVE_GCC_ERROR_RETURN_TYPE)
    set(CMAKE_CXX_FLAGS_ERROR "-Werror=return-type")
  endif()

  # If we are compiling on Linux then set some extra linker flags too
  if(CMAKE_SYSTEM_NAME MATCHES Linux)
    option(VTK_LINKER_FATAL_WARNINGS "Specify if linker warnings must be considered as errors." OFF)
    mark_as_advanced(VTK_LINKER_FATAL_WARNINGS)
    if(VTK_LINKER_FATAL_WARNINGS)
      set(VTK_EXTRA_SHARED_LINKER_FLAGS "-Wl,--fatal-warnings")
    endif()
    set(CMAKE_SHARED_LINKER_FLAGS
      "${VTK_EXTRA_SHARED_LINKER_FLAGS} -Wl,-lc ${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS
      "${VTK_EXTRA_SHARED_LINKER_FLAGS} -Wl,-lc ${CMAKE_SHARED_LINKER_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS
      "${VTK_EXTRA_SHARED_LINKER_FLAGS} -Wl,-lc ${CMAKE_SHARED_LINKER_FLAGS}")
  endif()

  # Set up the debug CXX_FLAGS for extra warnings
  option(VTK_EXTRA_COMPILER_WARNINGS
    "Add compiler flags to do stricter checking when building debug." OFF)
  if(VTK_EXTRA_COMPILER_WARNINGS)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
      "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CMAKE_CXX_FLAGS_WARN}")
    set(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS_WARN} ${CMAKE_CXX_FLAGS_ERROR}")
  endif()
endif()

# Intel OneAPI compilers >= 2021.2.0 turn on "fast math" at any non-zero
# optimization level. Suppress this non-standard behavior using the
# `-fp-model=precise` flag.
set(intel_oneapi_compiler_detections)
set(intel_oneapi_compiler_version_min "2021.2.0")
foreach (lang IN ITEMS C CXX Fortran)
  if (CMAKE_VERSION VERSION_LESS "3.14" AND lang STREQUAL "Fortran") # XXX(cmake-3.14): `Fortran_COMPILER_ID` genex
    continue ()
  endif ()
  # Detect the IntelLLVM compiler for the given language.
  set(is_lang "$<COMPILE_LANGUAGE:${lang}>")
  set(is_intelllvm "$<${lang}_COMPILER_ID:IntelLLVM>")
  set(is_intelllvm_fastmath_assuming_version "$<VERSION_GREATER_EQUAL:$<${lang}_COMPILER_VERSION>,${intel_oneapi_compiler_version_min}>")
  list(APPEND intel_oneapi_compiler_detections
    "$<AND:${is_lang},${is_intelllvm},${is_intelllvm_fastmath_assuming_version}>")
endforeach ()
string(REPLACE ";" "," intel_oneapi_compiler_detections "${intel_oneapi_compiler_detections}")
if (TARGET vtkbuild)
  target_compile_options(vtkbuild
    INTERFACE
      "$<BUILD_INTERFACE:$<$<OR:${intel_oneapi_compiler_detections}>:-fp-model=precise>>")
endif ()
