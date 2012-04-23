#
# a cmake implementation of the Wrap Tcl command
#

# VTK_MAKE_INSTANTIATOR(className
#                       outSourceList
#                       src-list1
#                       EXPORT_MACRO
#                       HEADER_LOCATION
#                       INCLUDES)
#
# Generates a new class with the given name and adds its files to the
# given outSourceList.  It registers the classes from the other given
# source lists with vtkInstantiator when it is loaded.  The output
# source list should be added to the library with the classes it
# registers.
# The EXPORT_MACRO argument must be given and followed by the export
# macro to use when generating the class (ex. VTK_COMMON_EXPORT).
# The HEADER_LOCATION option must be followed by a path.  It specifies
# the directory in which to place the generated class's header file.
# The generated class implementation files always go in the build
# directory corresponding to the CMakeLists.txt file containing
# the command.  This is the default location for the header.
# The INCLUDES option can be followed by a list of zero or more files.
# These files will be #included by the generated instantiator header,
# and can be used to gain access to the specified exportMacro in the
# C++ code.

MACRO(VTK_MAKE_INSTANTIATOR2 className outSourceList)
  # convert to the WRAP3 signature
  SET (SOURCES)
  SET (INCLUDES)
  SET (MODE SOURCES_MODE)
  FOREACH(ARG ${ARGN})
    SET (MODE_CHANGED 0)
    IF (ARG MATCHES INCLUDES)
      SET (MODE INCLUDES_MODE)
      SET (MODE_CHANGED 1)
    ENDIF (ARG MATCHES INCLUDES)
    IF (ARG MATCHES EXPORT_MACRO)
      SET (MODE EXPORT_MODE)
      SET (MODE_CHANGED 1)
    ENDIF (ARG MATCHES EXPORT_MACRO)
    IF (ARG MATCHES HEADER_LOCATION)
      SET (MODE HEADER_LOCATION_MODE)
      SET (MODE_CHANGED 1)
    ENDIF (ARG MATCHES HEADER_LOCATION)
    IF (ARG MATCHES INCLUDES)
      SET (MODE INCLUDES_MODE)
      SET (MODE_CHANGED 1)
    ENDIF (ARG MATCHES INCLUDES)
    IF (NOT MODE_CHANGED)
      IF (MODE MATCHES SOURCES_MODE)
        SET(SOURCES ${SOURCES} ${ARG})
      ENDIF (MODE MATCHES SOURCES_MODE)
      IF (MODE MATCHES INCLUDES_MODE)
        SET(INCLUDES ${INCLUDES} ${ARG})
      ENDIF (MODE MATCHES INCLUDES_MODE)
      IF (MODE MATCHES EXPORT_MODE)
        SET(EXPORT_MACRO "${ARG}")
      ENDIF (MODE MATCHES EXPORT_MODE)
      IF (MODE MATCHES HEADER_LOCATION_MODE)
        SET(HEADER_LOCATION "${ARG}")
      ENDIF (MODE MATCHES HEADER_LOCATION_MODE)
    ENDIF (NOT MODE_CHANGED)
  ENDFOREACH(ARG)
  VTK_MAKE_INSTANTIATOR3(${className} ${outSourceList} "${SOURCES}" "${EXPORT_MACRO}" "${HEADER_LOCATION}" "${INCLUDES}")
ENDMACRO(VTK_MAKE_INSTANTIATOR2)


MACRO(VTK_MAKE_INSTANTIATOR3 className outSourceList SOURCES EXPORT_MACRO HEADER_LOCATION INCLUDES)

  # Initialize local variables
  SET(HEADER_CONTENTS)
  SET(CXX_CONTENTS)
  SET(CXX_CONTENTS2)
  SET(CXX_CONTENTS3)

  # make the arguments available to the configured files
  SET (VTK_MAKE_INSTANTIATOR_CLASS_NAME ${className})
  SET (VTK_MAKE_INSTANTIATOR_EXPORT_MACRO ${EXPORT_MACRO})

  # For each include
  FOREACH(FILE ${INCLUDES})
    # generate the header
    SET (HEADER_CONTENTS
      "${HEADER_CONTENTS}#include \"${FILE}\"\n")
  ENDFOREACH(FILE)

  # For each class
  FOREACH(FILE ${SOURCES})

    # should we wrap the file?
    SET (WRAP_THIS_CLASS 1)
    GET_SOURCE_FILE_PROPERTY(TMP_WRAP_EXCLUDE ${FILE} WRAP_EXCLUDE)
    GET_SOURCE_FILE_PROPERTY(TMP_ABSTRACT ${FILE} ABSTRACT)

    # if it is abstract or wrap exclude then don't wrap it
    IF (TMP_WRAP_EXCLUDE OR TMP_ABSTRACT)
      SET (WRAP_THIS_CLASS 0)
    ENDIF (TMP_WRAP_EXCLUDE OR TMP_ABSTRACT)

    # don't wrap vtkIndent or vtkTimeStamp
    IF (${FILE} MATCHES "vtkIndent")
      SET (WRAP_THIS_CLASS 0)
    ENDIF (${FILE} MATCHES "vtkIndent")
    IF (${FILE} MATCHES "vtkTimeStamp")
      SET (WRAP_THIS_CLASS 0)
    ENDIF (${FILE} MATCHES "vtkTimeStamp")
    IF (${FILE} MATCHES "vtkVariant")
      SET (WRAP_THIS_CLASS 0)
    ENDIF (${FILE} MATCHES "vtkVariant")

    # finally if we should wrap it, then ...
    IF (WRAP_THIS_CLASS)

      # what is the filename without the extension
      GET_FILENAME_COMPONENT(TMP_FILENAME ${FILE} NAME_WE)

      # generate the implementation
      SET (CXX_CONTENTS
        "${CXX_CONTENTS}extern vtkObject* vtkInstantiator${TMP_FILENAME}New();\n")

      SET (CXX_CONTENTS2
        "${CXX_CONTENTS2}  vtkInstantiator::RegisterInstantiator(\"${TMP_FILENAME}\", vtkInstantiator${TMP_FILENAME}New);\n")

      SET (CXX_CONTENTS3
        "${CXX_CONTENTS3}  vtkInstantiator::UnRegisterInstantiator(\"${TMP_FILENAME}\", vtkInstantiator${TMP_FILENAME}New);\n")

    ENDIF (WRAP_THIS_CLASS)
  ENDFOREACH(FILE)

  # add the source file to the source list
  SET(${outSourceList} ${${outSourceList}}
    ${CMAKE_CURRENT_BINARY_DIR}/${className}.cxx)

  SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_BINARY_DIR}/${className}.cxx
    PROPERTIES GENERATED 1 WRAP_EXCLUDE 1 ABSTRACT 0
    )

  CONFIGURE_FILE(
    ${VTK_CMAKE_DIR}/vtkMakeInstantiator.h.in
    ${HEADER_LOCATION}/${className}.h
    COPY_ONLY
    IMMEDIATE
    )
  CONFIGURE_FILE(
    ${VTK_CMAKE_DIR}/vtkMakeInstantiator.cxx.in
    ${CMAKE_CURRENT_BINARY_DIR}/${className}.cxx
    COPY_ONLY
    IMMEDIATE
    )

ENDMACRO(VTK_MAKE_INSTANTIATOR3)
