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
    IF (VLI_INCLUDE_PATH_FOR_VG500)      
      SET(VLI_INCLUDE_DIRECTORIES ${VLI_INCLUDE_PATH_FOR_VG500} )
     ENDIF (VLI_INCLUDE_PATH_FOR_VG500)   
     IF (VLI_INCLUDE_PATH_FOR_VP1000)   
      SET(VLI_INCLUDE_DIRECTORIES ${VLI_INCLUDE_PATH_FOR_VP1000} )   
     ENDIF (VLI_INCLUDE_PATH_FOR_VP1000)   
     IF (VLI_INCLUDE_DIRECTORIES)
       INCLUDE_DIRECTORIES(${VLI_INCLUDE_DIRECTORIES})
     ELSE (VLI_INCLUDE_DIRECTORIES)
       MESSAGE("Could not find the include directories for the Volume Pro")
     ENDIF (VLI_INCLUDE_DIRECTORIES)

     IF (VLI_LIBRARY_FOR_VG500)   
       SET(VLI_LIBRARIES ${VLI_LIBRARY_FOR_VG500})
     ENDIF (VLI_LIBRARY_FOR_VG500)   
     IF (VLI_LIBRARY_FOR_VP1000)   
       SET(VLI_LIBRARIES ${VLI_LIBRARY_FOR_VP1000})   
     ENDIF (VLI_LIBRARY_FOR_VP1000) 
     IF (VLI_LIBRARIES)
       LINK_LIBRARIES (${VLI_LIBRARIES})
     ELSE (VLI_LIBRARIES)
       MESSAGE("Could not find the libraries for the Volume Pro")
     ENDIF (VLI_LIBRARIES)
  ENDIF (VTK_USE_VOLUMEPRO)
  INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Rendering)
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
  ELSE (VTK_USE_RENDERING)
    MESSAGE("The Hybrid option requires Rendering, but you do not have Rendering selected." "Warning")
  ENDIF (VTK_USE_RENDERING)
ENDIF (VTK_USE_HYBRID)

IF (VTK_USE_PATENTED)
  INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Patented)
ENDIF (VTK_USE_PATENTED)

IF (VTK_USE_PARALLEL)
  INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Parallel)
ENDIF (VTK_USE_PARALLEL)

INCLUDE_DIRECTORIES(
${VTK_BINARY_DIR} 
${VTK_SOURCE_DIR}/Common
${VTK_SOURCE_DIR}/Filtering
${VTK_SOURCE_DIR}/Imaging
${VTK_SOURCE_DIR}/Graphics
${VTK_SOURCE_DIR}/IO
)

IF (UNIX)
  LINK_LIBRARIES(${CMAKE_THREAD_LIBS} ${CMAKE_DL_LIBS} -lm)
ENDIF (UNIX)

#
# Look for the PNG, zlib, jpeg, and tiff libraries and header files
#
INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Utilities/zlib)
INCLUDE_DIRECTORIES(${VTK_BINARY_DIR}/Utilities/zlib)
INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Utilities/png)
INCLUDE_DIRECTORIES(${VTK_BINARY_DIR}/Utilities/png)
INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Utilities/jpeg)
INCLUDE_DIRECTORIES(${VTK_BINARY_DIR}/Utilities/jpeg)
INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Utilities/tiff)
INCLUDE_DIRECTORIES(${VTK_BINARY_DIR}/Utilities/tiff)
INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Utilities/expat)
INCLUDE_DIRECTORIES(${VTK_BINARY_DIR}/Utilities/expat)

#
# add include path for general testing header file
#
INCLUDE_DIRECTORIES(${VTK_SOURCE_DIR}/Common/Testing/Cxx)

IF(NOT CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_ANSI_CFLAGS}")
  IF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -timplicit_local -no_implicit_include ")
  ENDIF(CMAKE_SYSTEM MATCHES "OSF1-V.*")
ENDIF(NOT CMAKE_COMPILER_IS_GNUCXX)

IF(CMAKE_COMPILER_IS_GNUCXX)
  IF(WIN32)
     SET(CMAKE_NO_EXPLICIT_TEMPLATE_INSTANTIATION 1)
  ENDIF(WIN32)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(APPLE)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
  IF (VTK_USE_CARBON)
    SET(CMAKE_CXX_SHLIB_LINK_FLAGS "${CMAKE_CXX_SHLIB_LINK_FLAGS} -multiply_defined suppress -framework Carbon")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpascal-strings")
  ENDIF (VTK_USE_CARBON)
  IF (VTK_USE_COCOA)
    SET(CMAKE_CXX_SHLIB_LINK_FLAGS "${CMAKE_CXX_SHLIB_LINK_FLAGS} -multiply_defined suppress -framework Cocoa")
  ENDIF (VTK_USE_COCOA)
ENDIF(APPLE)

IF(CMAKE_COMPILER_IS_GNUCXX)
  IF(WIN32)
    IF(UNIX)
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwin32")
      SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mwin32")
    ENDIF(UNIX)
  ENDIF(WIN32)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF (OPENGL_INCLUDE_DIR)
  INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
ENDIF(OPENGL_INCLUDE_DIR)

# This one because OPENGL_INCLUDE_PATH is also defined for backward compatibility

IF (OPENGL_INCLUDE_PATH)
  INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_PATH})
ENDIF(OPENGL_INCLUDE_PATH)

#
# get information for Tcl wrapping 
#
IF (VTK_WRAP_TCL)
  # add in the Tcl values if found
  IF (TCL_INCLUDE_PATH)
    INCLUDE_DIRECTORIES(${TCL_INCLUDE_PATH})
  ELSE (TCL_INCLUDE_PATH)
    MESSAGE("Could not find the include directory for Tcl")
  ENDIF (TCL_INCLUDE_PATH)

  # The tcl package location.
  SET(VTK_TCL_HOME ${VTK_SOURCE_DIR}/Wrapping/Tcl)
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
  ELSE (JAVA_INCLUDE_PATH) 
    MESSAGE("Could not find the include directory for Java")
  ENDIF (JAVA_INCLUDE_PATH)        
  IF (JAVA_INCLUDE_PATH2)
    INCLUDE_DIRECTORIES(${JAVA_INCLUDE_PATH2})   
  ELSE (JAVA_INCLUDE_PATH2)   
    MESSAGE("Could not find the second include directory for Java")
  ENDIF (JAVA_INCLUDE_PATH2)   
  IF (JAVA_AWT_INCLUDE_PATH)   
    INCLUDE_DIRECTORIES(${JAVA_AWT_INCLUDE_PATH})   
  ELSE (JAVA_AWT_INCLUDE_PATH) 
    MESSAGE("Could not find the awt include directory for Java")
  ENDIF (JAVA_AWT_INCLUDE_PATH) 
ENDIF (VTK_WRAP_JAVA)

# The doxygen helper perl script location.
SET(VTK_DOXYGEN_HOME ${VTK_SOURCE_DIR}/Utilities/Doxygen)
