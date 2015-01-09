########################################################
#  Include file for user options
########################################################

# To use this option, copy both the macro and option code
# into the root UserMacros.cmake file. 
# OR add an include to the root UserMacros.cmake file:
# INCLUDE(path_to_file/WINDOWS_MT.cmake)

#-----------------------------------------------------------------------------
# Option to Build with Static CRT libraries on Windows
#-------------------------------------------------------------------------------
MACRO (TARGET_STATIC_CRT_FLAGS)
  if (MSVC AND NOT BUILD_SHARED_LIBS)
    foreach (flag_var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      if (${flag_var} MATCHES "/MD")
        string (REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      endif (${flag_var} MATCHES "/MD")
    endforeach (flag_var)
    foreach (flag_var
        CMAKE_Fortran_FLAGS CMAKE_Fortran_FLAGS_DEBUG CMAKE_Fortran_FLAGS_RELEASE
        CMAKE_Fortran_FLAGS_MINSIZEREL CMAKE_Fortran_FLAGS_RELWITHDEBINFO)
      if (${flag_var} MATCHES "/libs:dll")
        string (REGEX REPLACE "/libs:dll" "/libs:static" ${flag_var} "${${flag_var}}")
      endif (${flag_var} MATCHES "/libs:dll")
    endforeach (flag_var)
    set (WIN_COMPILE_FLAGS "")
    set (WIN_LINK_FLAGS "/NODEFAULTLIB:MSVCRT")
  endif (MSVC AND NOT BUILD_SHARED_LIBS)
ENDMACRO (TARGET_STATIC_CRT_FLAGS)

#-----------------------------------------------------------------------------
option (BUILD_STATIC_CRT_LIBS "Build With Static CRT Libraries" OFF)
if (BUILD_STATIC_CRT_LIBS)
  TARGET_STATIC_CRT_FLAGS ()
endif (BUILD_STATIC_CRT_LIBS)
 