CMAKE_MINIMUM_REQUIRED(VERSION 2.6.3 FATAL_ERROR)

# Set the project/library name here.  Classes should be declared as
# "class vtkLocal_EXPORT vtkFoo", where vtkLocal is the name of the
# project set here.
PROJECT(vtkLocal)

# Set your list of sources here.  Do not change the name of the
# vtkLocal_SRCS variable.
SET(vtkLocal_SRCS
  vtkLocalExample.cxx
)

# List the kits from VTK that are needed by this project.
SET(vtkLocal_LIBS
  vtkCommon
)

#-----------------------------------------------------------------------------
# Most users should not need to change anything below this line.

# Need to include class headers and the configuration header.
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})

# Setup output paths.
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH
    "Single output directory for building all libraries.")
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH
    "Single output directory for building all executables.")
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)

# Find and load VTK settings.
IF(NOT VTK_BINARY_DIR)
FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})
ENDIF(NOT VTK_BINARY_DIR)


# Give user option of building shared or static libraries.  Default to
# the choice made for VTK.
OPTION(BUILD_SHARED_LIBS "Build with shared libraries."
       ${VTK_BUILD_SHARED_LIBS})

# Set VTKLOCAL_BUILD_SHARED_LIBS to 1 or 0 for use in creating the
# configuration header.
IF(BUILD_SHARED_LIBS)
  SET(VTKLOCAL_BUILD_SHARED_LIBS_CONFIG 1)
ELSE(BUILD_SHARED_LIBS)
  SET(VTKLOCAL_BUILD_SHARED_LIBS_CONFIG 0)
ENDIF(BUILD_SHARED_LIBS)

IF(VTK_FOUND)
  # If this is a build tree, provide an option for putting this
  # project's executables and libraries in with VTK's.
  IF (EXISTS ${VTK_DIR}/bin)
    OPTION(USE_VTK_OUTPUT_PATHS
     "Use VTK's output directory for this project's executables and libraries."
     OFF)
    MARK_AS_ADVANCED (USE_VTK_OUTPUT_PATHS)
    IF (USE_VTK_OUTPUT_PATHS)
      SET (LIBRARY_OUTPUT_PATH ${VTK_DIR}/bin)
      SET (EXECUTABLE_OUTPUT_PATH ${VTK_DIR}/bin)
    ENDIF (USE_VTK_OUTPUT_PATHS)
  ENDIF (EXISTS ${VTK_DIR}/bin)

  # Create the header to configure the classes.
  CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/vtkLocalConfigure.h.in
                 ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Configure.h
                 @ONLY IMMEDIATE)

  # Use wrapping hints for this project.
  SET(VTK_WRAP_HINTS "${PROJECT_SOURCE_DIR}/hints")

  # Create the instantiator for these classes.
  SET(VTK_USE_INSTANTIATOR_NEW 1)
  VTK_MAKE_INSTANTIATOR3("${PROJECT_NAME}Instantiator"
    vtkLocalInstantiator_SRCS
    "${vtkLocal_SRCS}"
    "VTK_${PROJECT_NAME}_EXPORT"
    ${PROJECT_BINARY_DIR}
    "${PROJECT_NAME}Configure.h")

  # Create the library.
  ADD_LIBRARY(${PROJECT_NAME} ${vtkLocal_SRCS} ${vtkLocalInstantiator_SRCS})
  TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${vtkLocal_LIBS})

  # Create Tcl wrappers if VTK is wrapped in Tcl.
  IF(VTK_WRAP_TCL)
    INCLUDE(${VTK_CMAKE_DIR}/vtkWrapTcl.cmake)
    VTK_WRAP_TCL3(${PROJECT_NAME}TCL vtkLocalTCL_SRCS "${vtkLocal_SRCS}" "")
    ADD_LIBRARY(${PROJECT_NAME}TCL ${vtkLocalTCL_SRCS})
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}TCL ${PROJECT_NAME})
    FOREACH(c ${vtkLocal_LIBS})
      TARGET_LINK_LIBRARIES(${PROJECT_NAME}TCL ${c}TCL)
    ENDFOREACH(c)

    # Create the list of kits that are required.
    SET(VTKLOCAL_KITS "")
    FOREACH(c ${vtkLocal_LIBS})
      SET(VTKLOCAL_KITS "${VTKLOCAL_KITS} ${c}")
    ENDFOREACH(c)
    FOREACH(c ${vtkLocal_SRCS})
      GET_FILENAME_COMPONENT(VTKLOCAL_LAST_CLASS ${c} NAME_WE)
    ENDFOREACH(c)

    IF(CMAKE_CONFIGURATION_TYPES)
      FOREACH(config ${CMAKE_CONFIGURATION_TYPES})
        SET(VTKLOCAL_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}/${config}")
        CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/pkgIndex.tcl.in
                       ${PROJECT_BINARY_DIR}/${config}/pkgIndex.tcl
                       @ONLY IMMEDIATE)
      ENDFOREACH(config)
    ELSE(CMAKE_CONFIGURATION_TYPES)
      SET(VTKLOCAL_TCL_LIBRARY_DIR "${LIBRARY_OUTPUT_PATH}")
      CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/pkgIndex.tcl.in
                     ${PROJECT_BINARY_DIR}/pkgIndex.tcl
                     @ONLY IMMEDIATE)
    ENDIF(CMAKE_CONFIGURATION_TYPES)

  ENDIF(VTK_WRAP_TCL)

  # Create Python wrappers if VTK is wrapped in Python.
  IF(VTK_WRAP_PYTHON)
    SET(VTK_WRAP_PYTHON_FIND_LIBS ON)
    INCLUDE(${VTK_CMAKE_DIR}/vtkWrapPython.cmake)
    INCLUDE_DIRECTORIES("${PYTHON_INCLUDE_PATH}")
    VTK_WRAP_PYTHON3(${PROJECT_NAME}Python vtkLocalPYTHON_SRCS "${vtkLocal_SRCS}")
    ADD_LIBRARY(${PROJECT_NAME}PythonD ${vtkLocalPYTHON_SRCS})
    ADD_LIBRARY(${PROJECT_NAME}Python MODULE ${PROJECT_NAME}PythonInit.cxx)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}PythonD ${PROJECT_NAME})
    FOREACH(c ${vtkLocal_LIBS})
      TARGET_LINK_LIBRARIES(${PROJECT_NAME}PythonD ${c}PythonD)
    ENDFOREACH(c)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}Python ${PROJECT_NAME}PythonD)
    SET_TARGET_PROPERTIES(${PROJECT_NAME}Python PROPERTIES PREFIX "")
    IF(WIN32 AND NOT CYGWIN)
      SET_TARGET_PROPERTIES(${PROJECT_NAME}Python PROPERTIES SUFFIX ".pyd")
    ENDIF(WIN32 AND NOT CYGWIN)
  ENDIF(VTK_WRAP_PYTHON)

  # Create Java wrappers if VTK is wrapped in Java.
  IF(VTK_WRAP_JAVA)
    INCLUDE(${VTK_CMAKE_DIR}/vtkWrapJava.cmake)
    SET(VTK_WRAP_JAVA3_INIT_DIR "${PROJECT_SOURCE_DIR}")
    SET(VTK_JAVA_HOME ${PROJECT_BINARY_DIR}/java/${PROJECT_NAME})
    FILE(MAKE_DIRECTORY ${VTK_JAVA_HOME})
    VTK_WRAP_JAVA3(${PROJECT_NAME}Java vtkLocalJAVA_SRCS "${vtkLocal_SRCS}")
    ADD_LIBRARY(${PROJECT_NAME}Java ${vtkLocalJAVA_SRCS})
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}Java ${PROJECT_NAME})
    FOREACH(c ${vtkLocal_LIBS})
      TARGET_LINK_LIBRARIES(${PROJECT_NAME}Java ${c}Java)
    ENDFOREACH(c)
  ENDIF(VTK_WRAP_JAVA)

  ADD_SUBDIRECTORY(Testing)
ENDIF(VTK_FOUND)
