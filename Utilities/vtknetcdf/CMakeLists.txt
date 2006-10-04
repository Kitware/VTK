PROJECT(VTKNETCDF)

INCLUDE_REGULAR_EXPRESSION(".*")

SET(netcdf_SRCS
  attr.c
  dim.c
  error.c
  libvers.c
  nc.c
  ncio.c
  ncx.c
  putget.c
  string.c
  v1hpg.c
  v2i.c
  var.c
  )

IF(APPLE)
  SET_SOURCE_FILES_PROPERTIES(
    v2i.c
    PROPERTIES
    COMPILE_FLAGS -fno-common)
ENDIF(APPLE)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

# Include all the necessary files for macros
INCLUDE (CheckIncludeFiles)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckTypeSize)
INCLUDE (CheckSymbolExists)
INCLUDE (TestBigEndian)

MACRO(MANGLE_VARIABLE_NAME str var prefix)
  STRING(TOUPPER "${str}" mangle_variable_name_var)
  STRING(REGEX REPLACE "[/. ]" "_" mangle_variable_name_var "${mangle_variable_name_var}")
  SET(${var} "${prefix}${mangle_variable_name_var}")
ENDMACRO(MANGLE_VARIABLE_NAME str var)

# Check if header file exists and add it to the list.
MACRO(CHECK_INCLUDE_FILE_CONCAT FILE)
  MANGLE_VARIABLE_NAME("${FILE}" "CHECK_INCLUDE_FILE_CONCAT_VAR" "HAVE_")
  CHECK_INCLUDE_FILES("${HEADER_INCLUDES};${FILE}" ${CHECK_INCLUDE_FILE_CONCAT_VAR})
  IF(${CHECK_INCLUDE_FILE_CONCAT_VAR})
    SET(HEADER_INCLUDES ${HEADER_INCLUDES} ${FILE})
  ENDIF(${CHECK_INCLUDE_FILE_CONCAT_VAR})
ENDMACRO(CHECK_INCLUDE_FILE_CONCAT)

MACRO(CHECK_FUNCTION_EXISTS_EX FUNC)
  MANGLE_VARIABLE_NAME("${FUNC}" "CHECK_FUNCTION_EXISTS_EX_VAR" "HAVE_")
  CHECK_FUNCTION_EXISTS("${FUNC}" "${CHECK_FUNCTION_EXISTS_EX_VAR}")
ENDMACRO(CHECK_FUNCTION_EXISTS_EX)

MACRO(CHECK_SYMBOL_EXISTS_EX SYM)
  MANGLE_VARIABLE_NAME("${SYM}" "CHECK_SYMBOL_EXISTS_EX_VAR" "HAVE_")
  CHECK_SYMBOL_EXISTS("${SYM}" "${HEADER_INCLUDES}" "${CHECK_SYMBOL_EXISTS_EX_VAR}")
ENDMACRO(CHECK_SYMBOL_EXISTS_EX)

MACRO(CHECK_TYPE_SIZE_EX type)
  MANGLE_VARIABLE_NAME("${type}" "check_type_size_var" "")
  CHECK_TYPE_SIZE("${type}" "SIZEOF_${check_type_size_var}")
  IF(HAVE_${check_type_size_var})
    SET("HAVE_${check_type_size_var}" 1)
  ENDIF(HAVE_${check_type_size_var})
ENDMACRO(CHECK_TYPE_SIZE_EX)

MACRO(C_SOURCE_COMPILES SOURCE VAR)
  IF("${VAR}" MATCHES "^${VAR}$")
    SET(MACRO_CHECK_FUNCTION_DEFINITIONS 
      "-D${VAR} ${CMAKE_REQUIRED_FLAGS}")
    IF(CMAKE_REQUIRED_LIBRARIES)
      SET(C_SOURCE_COMPILES_ADD_LIBRARIES
        "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    ENDIF(CMAKE_REQUIRED_LIBRARIES)
    FILE(WRITE "${CMAKE_BINARY_DIR}/CMakeTmp/src.c"
      "${SOURCE}")

    MESSAGE(STATUS "Performing NetCDF Test ${VAR}")
    TRY_COMPILE(${VAR}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_BINARY_DIR}/CMakeTmp/src.c
      CMAKE_FLAGS 
      "${C_SOURCE_COMPILES_ADD_LIBRARIES}"
      OUTPUT_VARIABLE OUTPUT)
    IF(${VAR})
      SET(${VAR} 1 CACHE INTERNAL "NetCDF test ${FUNCTION}")
      MESSAGE(STATUS "Performing NetCDF Test ${VAR} - Success")
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeOutput.log 
        "Performing C SOURCE FILE Test ${VAR} succeded with the following output:\n"
        "${OUTPUT}\n"
        "Source file was:\n${SOURCE}\n" APPEND)
    ELSE(${VAR})
      MESSAGE(STATUS "Performing NetCDF Test ${VAR} - Failed")
      SET(${VAR} "" CACHE INTERNAL "NetCDF test ${FUNCTION}")
      WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log 
        "Performing C SOURCE FILE Test ${VAR} failed with the following output:\n"
        "${OUTPUT}\n"
        "Source file was:\n${SOURCE}\n" APPEND)
    ENDIF(${VAR})
  ENDIF("${VAR}" MATCHES "^${VAR}$")
ENDMACRO(C_SOURCE_COMPILES)

CHECK_INCLUDE_FILES("stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS)
FOREACH(file 
    "alloca.h"
    "stdlib.h"
    "sys/types.h"
    "sys/stat.h"
    "unistd.h"
    "fcntl.h"
    "stdio.h"
    "string.h"
    "stddef.h"
    "stdint.h"
    )
  CHECK_INCLUDE_FILE_CONCAT("${file}")
ENDFOREACH(file)
FOREACH(func
    alloca
    strerror
    )
  CHECK_SYMBOL_EXISTS_EX("${func}")
ENDFOREACH(func)
FOREACH(type
    "size_t"
    "ssize_t"
    "ptrdiff_t"
    "off_t"
    "double"
    "float"
    "int"
    "long"
    "short"
    "uchar"
    )
  CHECK_TYPE_SIZE_EX("${type}")
ENDFOREACH(type)

SET(testsrc
"#include <sys/stat.h>
int main() { return 0; }
int t() {
struct stat s; s.st_blksize;
; return 0; }")
IF(HAVE_SYS_TYPES_H)
  SET(testsrc "#include <sys/types.h>\n${testsrc}")
ENDIF(HAVE_SYS_TYPES_H)

IF(HAVE_SYS_STAT_H)
  C_SOURCE_COMPILES("${testsrc}" HAVE_ST_BLKSIZE)
ENDIF(HAVE_SYS_STAT_H)

# 
SET(testsrc
  "main()
{
  char*   path = tmpnam(NULL);
  int     exitStatus = 1;

  if (path != NULL)
    {
    int   fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);

    if (fd != -1)
      {
      if (write(fd, \"0\", 1) == 1)
        {
        off_t   pos = lseek(fd, 0, SEEK_CUR);

        if (pos != (off_t)-1)
          {
          if (ftruncate(fd, 512) != -1)
            {
            if (pos == lseek(fd, 0, SEEK_CUR))
              {
              if (lseek(fd, 0, SEEK_SET) == 0)
                {
                char  buf[512];

                if (read(fd, buf, 512) == 512)
                  exitStatus = 0;
                }
              }
            }
          }
        }
      close(fd);
      unlink(path);
      }
    }

  return exitStatus;
}")
IF(HAVE_SYS_TYPES_H)
  SET(testsrc "#include <sys/types.h>\n${testsrc}")
ENDIF(HAVE_SYS_TYPES_H)
IF(HAVE_SYS_STAT_H)
  SET(testsrc "#include <sys/stat.h>\n${testsrc}")
ENDIF(HAVE_SYS_STAT_H)
IF(HAVE_FCNTL_H)
  SET(testsrc "#include <fcntl.h>\n${testsrc}")
ENDIF(HAVE_FCNTL_H)
IF(HAVE_STDIO_H)
  SET(testsrc "#include <stdio.h>\n${testsrc}")
ENDIF(HAVE_STDIO_H)
IF(HAVE_UNISTD_H)
  SET(testsrc "#include <unistd.h>\n${testsrc}")
ENDIF(HAVE_UNISTD_H)
C_SOURCE_COMPILES("${testsrc}" HAVE_FTRUNCATE)

MACRO(INVERT_VARIABLE var val)
  SET(INVERT_VARIABLE_VAR "${val}")
  SET(${var} "1")
  IF(INVERT_VARIABLE_VAR)
    SET(${var} "0")
  ENDIF(INVERT_VARIABLE_VAR)
ENDMACRO(INVERT_VARIABLE)

INVERT_VARIABLE(NO_STDLIB_H "${HAVE_STDLIB_H}")
INVERT_VARIABLE(NO_SYS_TYPES_H "${HAVE_SYS_TYPES_H}")
INVERT_VARIABLE(NO_STRERROR "${HAVE_STRERROR}")

IF(NOT HAVE_SIZEOF_SIZE_T)
  SET(size_t long)
ENDIF(NOT HAVE_SIZEOF_SIZE_T)
IF(NOT HAVE_SIZEOF_OFF_T)
  SET(off_t long)
ENDIF(NOT HAVE_SIZEOF_OFF_T)
IF(NOT HAVE_SIZEOF_SSIZE_T)
  SET(ssize_t int)
ENDIF(NOT HAVE_SIZEOF_SSIZE_T)
IF(NOT HAVE_SIZEOF_PTRDIFF_T)
  SET(ptrdiff_t int)
ENDIF(NOT HAVE_SIZEOF_PTRDIFF_T)
IF(NOT HAVE_SIZEOF_UCHAR)
  SET(uchar "unsigned char")
ENDIF(NOT HAVE_SIZEOF_UCHAR)

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/ncconfig.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/ncconfig.h @ONLY IMMEDIATE)

ADD_LIBRARY(vtkNetCDF
  ${netcdf_SRCS})

# Apply user-defined properties to the library target.
IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(vtkNetCDF PROPERTIES ${VTK_LIBRARY_PROPERTIES})
ENDIF(VTK_LIBRARY_PROPERTIES)

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS vtkNetCDF
    RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
    LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${VTKNETCDF_BINARY_DIR}/ncconfig.h
    ${VTKNETCDF_SOURCE_DIR}/netcdf.h
    ${VTKNETCDF_SOURCE_DIR}/vtk_netcdf_mangle.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/vtknetcdf
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
