#
# This file can be included by other projects that use or depend on VTK
# it sets up many default parameters and include paths.
# Please do not add any commands which creates/sets cache entries
# in this file since it is usually used in combination with the
# LOAD_CACHE() command. Otherwise, the same entry will be set
# more than once and the results will be unpredictable.
#

IF (VTK_USE_RENDERING)
  IF (VTK_USE_VOLUMEPRO)
    IF (VLI_INCLUDE_PATH)
      INCLUDE_DIRECTORIES( ${VLI_INCLUDE_PATH} )
    ENDIF (VLI_INCLUDE_PATH)
    IF (VLI_LIBRARY)
      LINK_LIBRARIES (${VLI_LIBRARY})
    ENDIF (VLI_LIBRARY)
  ENDIF (VTK_USE_VOLUMEPRO)
  INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Rendering)
  LINK_DIRECTORIES(${VTK_BINARY_DIR}/Rendering) 
ENDIF (VTK_USE_RENDERING)

# Optional Stanford University PLY file format
IF(PLY_LIBRARY)
  IF(PLY_PATH)
    LINK_LIBRARIES( ${PLY_LIBRARY} )
  ENDIF(PLY_PATH)
ENDIF(PLY_LIBRARY)

IF (VTK_USE_HYBRID)
  # hybrid requires rendering
  IF (VTK_USE_RENDERING)
    INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Hybrid)
    LINK_DIRECTORIES(${VTK_BINARY_DIR}/Hybrid) 
  ELSE (VTK_USE_RENDERING)
    MESSAGE("The Hybrid option requires Rendering, but you do not have Rendering selected." "Warning")
  ENDIF (VTK_USE_RENDERING)
ENDIF (VTK_USE_HYBRID)

IF (VTK_USE_PATENTED)
  INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Patented)
  LINK_DIRECTORIES(${VTK_BINARY_DIR}/Patented) 
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_PARALLEL)
    INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Parallel)
    LINK_DIRECTORIES(${VTK_BINARY_DIR}/Parallel) 
ENDIF (VTK_USE_PARALLEL)

#
# get information for Tcl wrapping 
#
IF (VTK_WRAP_TCL)
  # add in the Tcl values if found
  IF (TCL_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${TCL_INCLUDE_PATH})
  ENDIF (TCL_INCLUDE_PATH)
ENDIF (VTK_WRAP_TCL)

#
# get information for Python wrapping 
#
IF (VTK_WRAP_PYTHON)
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
ENDIF (VTK_WRAP_PYTHON)

#
# get information for Java wrapping 
#
IF (VTK_WRAP_JAVA)
  IF (JAVA_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH})
  ENDIF (JAVA_INCLUDE_PATH)
  IF (JAVA_INCLUDE_PATH2)
    INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH2})
  ENDIF (JAVA_INCLUDE_PATH2)
  IF (JAVA_AWT_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${JAVA_AWT_INCLUDE_PATH})
  ENDIF (JAVA_AWT_INCLUDE_PATH)
ENDIF (VTK_WRAP_JAVA)

INCLUDE_DIRECTORIES(
${VTK_BINARY_DIR} 
${VTK_SOURCE_DIR}/Common
${VTK_SOURCE_DIR}/Filtering
${VTK_SOURCE_DIR}/Imaging
${VTK_SOURCE_DIR}/Graphics
${VTK_SOURCE_DIR}/IO
)

IF (NOT LIBRARY_OUTPUT_PATH)
  LINK_DIRECTORIES(
    ${VTK_BINARY_DIR}/Common 
    ${VTK_BINARY_DIR}/Filtering
    ${VTK_BINARY_DIR}/Imaging
    ${VTK_BINARY_DIR}/Graphics
    ${VTK_BINARY_DIR}/IO
    )
ENDIF (NOT LIBRARY_OUTPUT_PATH)

IF (UNIX)
  LINK_LIBRARIES(${CMAKE_THREAD_LIBS} ${CMAKE_DL_LIBS} -lm)
ENDIF (UNIX)

#
# Look for the PNG and zlib libraries and header files
#
INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Utilities/zlib)
INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Utilities/png)
IF (NOT LIBRARY_OUTPUT_PATH)
  LINK_DIRECTORIES(${VTK_BINARY_DIR}/Utilities/png)
  LINK_DIRECTORIES(${VTK_BINARY_DIR}/Utilities/zlib)
ENDIF (NOT LIBRARY_OUTPUT_PATH)

#
# add include path for general testing header file
#
IF (BUILD_TESTING)
  INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Common/Testing/Cxx)
ENDIF (BUILD_TESTING)

IF(CMAKE_COMPILER_IS_GNUCXX)
  IF(WIN32)
    IF(UNIX)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwin32")
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mwin32")
    ENDIF(UNIX)
  ENDIF(WIN32)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

