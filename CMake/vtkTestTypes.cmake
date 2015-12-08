SET(VTK_SIZEOF_VOID_P  ${CMAKE_SIZEOF_VOID_P})

INCLUDE (CheckTypeSize)
CHECK_TYPE_SIZE(int      VTK_SIZEOF_INT)
CHECK_TYPE_SIZE(long     VTK_SIZEOF_LONG)
CHECK_TYPE_SIZE(char     VTK_SIZEOF_CHAR)
CHECK_TYPE_SIZE(short    VTK_SIZEOF_SHORT)
CHECK_TYPE_SIZE(float    VTK_SIZEOF_FLOAT)
CHECK_TYPE_SIZE(double   VTK_SIZEOF_DOUBLE)

CHECK_TYPE_SIZE("long long" VTK_SIZEOF_LONG_LONG)

# Enable the "long long" type if it is available.  It is standard in
# C99 and C++03 but not in earlier standards.
SET(VTK_TYPE_USE_LONG_LONG)
IF(VTK_SIZEOF_LONG_LONG)
  SET(VTK_TYPE_USE_LONG_LONG 1)
ENDIF()

IF(NOT DEFINED VTK_TYPE_CHAR_IS_SIGNED)
  MESSAGE(STATUS "Checking signedness of char")
  TRY_RUN(VTK_TYPE_CHAR_IS_SIGNED VTK_TYPE_CHAR_IS_SIGNED_COMPILED
          ${VTK_BINARY_DIR}/CMakeTmp/Char
          ${VTK_CMAKE_DIR}/vtkTestCharSignedness.cxx)
  IF(VTK_TYPE_CHAR_IS_SIGNED_COMPILED)
    IF(VTK_TYPE_CHAR_IS_SIGNED)
      MESSAGE(STATUS "Checking signedness of char -- signed")
      SET(VTK_TYPE_CHAR_IS_SIGNED 1 CACHE INTERNAL "Whether char is signed.")
    ELSE()
      MESSAGE(STATUS "Checking signedness of char -- unsigned")
      SET(VTK_TYPE_CHAR_IS_SIGNED 0 CACHE INTERNAL "Whether char is signed.")
    ENDIF()
  ELSE()
    MESSAGE(STATUS "Checking signedness of char -- failed")
  ENDIF()
ENDIF()
