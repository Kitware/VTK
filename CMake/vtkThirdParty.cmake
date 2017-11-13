# Do we try to use system libraries by default?
option(VTK_USE_SYSTEM_LIBRARIES "Use the system's libraries by default." OFF)
mark_as_advanced(VTK_USE_SYSTEM_LIBRARIES)

# TODO: This should be removed in the future. Warnings should be suppressed as
# part of the import process, not from this macro.
macro(vtk_third_party_warning_suppress upper lang)
  if(NOT ${upper}_WARNINGS_ALLOW)
    # Visual Studio generators of CMake use /W0 to suppress warnings.
    # MSVC complains if another warning level is given, so remove it.
    if(MSVC)
      set(${upper}_WARNINGS_BLOCKED 1)
      string(REGEX REPLACE "(^| )([/-])W[0-9]( |$)" " "
        CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS}")
      set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} /W0")
    endif()

    # Most compilers use -w to suppress warnings.
    if(NOT ${upper}_WARNINGS_BLOCKED)
      set(CMAKE_${lang}_FLAGS "${CMAKE_${lang}_FLAGS} -w")
    endif()
  endif()
endmacro()
