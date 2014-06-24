if(CMAKE_COMPILER_IS_GNUCXX)

  include(CheckCXXCompilerFlag)

  # Addtional warnings for GCC
  set(CMAKE_CXX_FLAGS_WARN "-Wnon-virtual-dtor -Wno-long-long -ansi -Wcast-align -Wchar-subscripts -Wall -Wextra -Wpointer-arith -Wformat-security -Woverloaded-virtual -Wshadow -Wunused-parameter -fno-check-new -fno-common")

  # This flag is useful as not returning from a non-void function is an error
  # with MSVC, but it is not supported on all GCC compiler versions
  check_cxx_compiler_flag(-Werror=return-type HAVE_GCC_ERROR_RETURN_TYPE)
  if(HAVE_GCC_ERROR_RETURN_TYPE)
    set(CMAKE_CXX_FLAGS_ERROR "-Werror=return-type")
  endif()

  # If we are compiling on Linux then set some extra linker flags too
  if(CMAKE_SYSTEM_NAME MATCHES Linux)
    set(CMAKE_SHARED_LINKER_FLAGS
      "-Wl,--fatal-warnings -Wl,--no-undefined -lc ${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS
      "-Wl,--fatal-warnings -Wl,--no-undefined -lc ${CMAKE_SHARED_LINKER_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS
      "-Wl,--fatal-warnings -Wl,--no-undefined -lc ${CMAKE_SHARED_LINKER_FLAGS}")
  endif()

  # Now check if we can use visibility to selectively export symbols
  execute_process(COMMAND ${CMAKE_C_COMPILER} ARGS --version
    OUTPUT_VARIABLE _gcc_version_info
    ERROR_VARIABLE _gcc_version_info)

  string (REGEX MATCH "[345]\\.[0-9]\\.[0-9]*"
    _gcc_version "${_gcc_version_info}")
  if(NOT _gcc_version)
    string (REGEX REPLACE ".*\\(GCC\\).*([34]\\.[0-9]).*" "\\1.0"
      _gcc_version "${_gcc_version_info}")
  endif()

  # GCC visibility support, on by default and in testing.
  check_cxx_compiler_flag(-fvisibility=hidden HAVE_GCC_VISIBILITY)
  option(VTK_USE_GCC_VISIBILITY "Use GCC visibility support if available." OFF)
  mark_as_advanced(VTK_USE_GCC_VISIBILITY)

  if(${_gcc_version} VERSION_GREATER 4.2.0 AND BUILD_SHARED_LIBS
    AND HAVE_GCC_VISIBILITY AND VTK_USE_GCC_VISIBILITY
    AND NOT MINGW AND NOT CYGWIN)
    # Should only be set if GCC is newer than 4.2.0
    set(VTK_ABI_CXX_FLAGS "-fvisibility=hidden -fvisibility-inlines-hidden")
  else()
    set(VTK_ABI_CXX_FLAGS "")
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

