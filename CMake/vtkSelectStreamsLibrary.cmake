# Include CHECK_INCLUDE_FILE_CXX macro used by this macro.
INCLUDE (${CMAKE_ROOT}/Modules/CheckIncludeFileCXX.cmake)

#-----------------------------------------------------------------------------
# Macro to select whether to use old streams or ANSI streams.
# Argument is the variable to set with the result (VTK_USE_ANSI_STDLIB).
MACRO(VTK_SELECT_STREAMS_LIBRARY var VTK_SOURCE_DIR)
  IF("${var}" MATCHES "^${var}$")
    CHECK_INCLUDE_FILE_CXX("iosfwd" VTK_HAVE_ANSI_STREAMS)
    CHECK_INCLUDE_FILE_CXX("iostream.h" VTK_HAVE_OLD_STREAMS)
    IF(VTK_HAVE_OLD_STREAMS)
      # Some compilers have iostream.h but not strstream.h
      # or strstrea.h.  Treat these as not having old streams.
      CHECK_INCLUDE_FILE_CXX("strstrea.h" VTK_HAVE_OLD_STRSTREA_H)
      CHECK_INCLUDE_FILE_CXX("strstream.h" VTK_HAVE_OLD_STRSTREAM_H)
      IF(NOT VTK_HAVE_OLD_STRSTREAM_H)
        IF(NOT VTK_HAVE_OLD_STRSTREA_H)
          SET(VTK_HAVE_OLD_STREAMS 0)
        ENDIF(NOT VTK_HAVE_OLD_STRSTREA_H)
      ENDIF(NOT VTK_HAVE_OLD_STRSTREAM_H)
    ENDIF(VTK_HAVE_OLD_STREAMS)

    IF(VTK_HAVE_ANSI_STREAMS AND VTK_HAVE_OLD_STREAMS)
      # Have both old and new streams.  Provide the option.
      # Default to OFF for MSVC 6 and ON for all others.
      SET(${var}_DEFAULT ON)
      IF(NOT CMAKE_COMPILER_IS_GNUCXX)
        IF("VTK_COMPILER_IS_VC6" MATCHES "^VTK_COMPILER_IS_VC6$")
          MESSAGE(STATUS "Checking if compiler is VC6")
          TRY_COMPILE(VTK_COMPILER_IS_VC6
            ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp
            ${VTK_SOURCE_DIR}/CMake/vtkTestCompilerIsVC6.cxx
            OUTPUT_VARIABLE OUTPUT)
          IF(VTK_COMPILER_IS_VC6)
            MESSAGE(STATUS "Checking if compiler is VC6 -- yes")
            SET(VTK_COMPILER_IS_VC6 1 CACHE INTERNAL "Compiler is MSVC 6")
          ELSE(VTK_COMPILER_IS_VC6)
            MESSAGE(STATUS "Checking if compiler is VC6 -- no")
            SET(VTK_COMPILER_IS_VC6 0 CACHE INTERNAL "Compiler is MSVC 6")
            WRITE_FILE(${CMAKE_BINARY_DIR}/CMakeFiles/CMakeError.log
              "Determining if the compiler is MSVC 6 "
              "failed with the following output:\n"
              "${OUTPUT}\n" APPEND)
          ENDIF(VTK_COMPILER_IS_VC6)
        ENDIF("VTK_COMPILER_IS_VC6" MATCHES "^VTK_COMPILER_IS_VC6$")
        IF(VTK_COMPILER_IS_VC6)
          SET(${var}_DEFAULT OFF)
        ENDIF(VTK_COMPILER_IS_VC6)
      ENDIF(NOT CMAKE_COMPILER_IS_GNUCXX)
      OPTION(${var} "Use the ANSI standard iostream library." ${${var}_DEFAULT})
      MARK_AS_ADVANCED(${var})
    ELSE(VTK_HAVE_ANSI_STREAMS AND VTK_HAVE_OLD_STREAMS)
      IF(VTK_HAVE_ANSI_STREAMS)
        # Have only new streams.  Use them.
        SET(${var} ON)
      ELSE(VTK_HAVE_ANSI_STREAMS)
        # Have only old streams.  Use them.
        SET(${var} OFF)
      ENDIF(VTK_HAVE_ANSI_STREAMS)
    ENDIF(VTK_HAVE_ANSI_STREAMS AND VTK_HAVE_OLD_STREAMS)
  ENDIF("${var}" MATCHES "^${var}$")
ENDMACRO(VTK_SELECT_STREAMS_LIBRARY var)
