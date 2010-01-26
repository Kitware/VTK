# Setup vtkInstantiator registration for this library's classes.
VTK_MAKE_INSTANTIATOR3(vtk${KIT}Instantiator KitInstantiator_SRCS
                       "${Kit_SRCS}"
                       VTK_${UKIT}_EXPORT
                       ${VTK_BINARY_DIR} "")

ADD_LIBRARY(vtk${KIT} ${Kit_SRCS} ${Kit_EXTRA_SRCS} ${KitInstantiator_SRCS})
SET(KIT_LIBRARY_TARGETS ${KIT_LIBRARY_TARGETS} vtk${KIT})
# Add target specific compile flags for ABI setttings
IF(VTK_ABI_CXX_FLAGS)
  SET_PROPERTY(TARGET vtk${KIT} APPEND 
    PROPERTY COMPILE_FLAGS "${VTK_ABI_CXX_FLAGS}")
ENDIF(VTK_ABI_CXX_FLAGS)

# Allow the user to customize their build with some local options
#
SET(LOCALUSERMACRODEFINED 0)
INCLUDE (${VTK_BINARY_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)
INCLUDE (${VTK_SOURCE_DIR}/${KIT}/LocalUserOptions.cmake OPTIONAL)

# if we are wrapping into Tcl then add the library and extra
# source files
#
IF (VTK_WRAP_TCL)
  INCLUDE(KitCommonTclWrapBlock)
ENDIF (VTK_WRAP_TCL)

# if we are wrapping into Python then add the library and extra
# source files
#
IF (VTK_WRAP_PYTHON)
  INCLUDE(KitCommonPythonWrapBlock)
ENDIF (VTK_WRAP_PYTHON)

# if we are wrapping into Java then add the library and extra
# source files
#
IF (VTK_WRAP_JAVA)
  INCLUDE(KitCommonJavaWrapBlock)
ENDIF (VTK_WRAP_JAVA)

TARGET_LINK_LIBRARIES(vtk${KIT} ${KIT_INTERFACE_LIBRARIES} ${KIT_LIBS})

IF(NOT VTK_INSTALL_NO_LIBRARIES)
  INSTALL(TARGETS vtk${KIT} 
    RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
    LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
    ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  SET(__files.h)
  FOREACH(__file ${Kit_SRCS})
    GET_FILENAME_COMPONENT(__fname ${__file} ABSOLUTE)
    GET_FILENAME_COMPONENT(__fname.path ${__fname} PATH)
    GET_FILENAME_COMPONENT(__fname.name ${__fname} NAME_WE)
    IF ( EXISTS "${__fname.path}/${__fname.name}.h" )
      SET(__files.h ${__files.h} "${__fname.path}/${__fname.name}.h")
    ENDIF ( EXISTS "${__fname.path}/${__fname.name}.h" )
  ENDFOREACH(__file)
  INSTALL(FILES ${__files.h}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)

VTK_EXPORT_KIT("${KIT}" "${UKIT}" "${Kit_SRCS}")

# If the user defined a custom macro, execute it now and pass in all the srcs
#
IF(LOCALUSERMACRODEFINED)
  LocalUserOptionsMacro( "${Kit_SRCS}"       "${Kit_EXTRA_SRCS}"
                         "${KitTCL_SRCS}"    "${Kit_TCL_EXTRA_SRCS}"
                         "${KitJava_SRCS}"   "${Kit_JAVA_EXTRA_SRCS}"
                         "${KitPython_SRCS}" "${Kit_PYTHON_EXTRA_SRCS}")
ENDIF(LOCALUSERMACRODEFINED)


# Apply user-defined properties to the library targets.
IF(VTK_LIBRARY_PROPERTIES)
  SET_TARGET_PROPERTIES(${KIT_LIBRARY_TARGETS} PROPERTIES
    ${VTK_LIBRARY_PROPERTIES}
    )
ENDIF(VTK_LIBRARY_PROPERTIES)
# Mark KIT_INTERFACE_LIBRARIES as LINK_INTERFACE_LIBRARIES such that any executable
# or library that links vtk${KIT} also links against KIT_INTERFACE_LIBRARIES
TARGET_LINK_LIBRARIES(vtk${KIT} LINK_INTERFACE_LIBRARIES ${KIT_INTERFACE_LIBRARIES})

