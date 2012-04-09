if(VTK_WRAP_JAVA)
  set(VTK_WRAP_JAVA3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping")
  find_package(Java)
  find_package(JNI)
  include(vtkWrapJava)
  
  IF(JAVA_AWT_LIBRARY)
  FOREACH(__java_library ${JAVA_AWT_LIBRARY})
    GET_FILENAME_COMPONENT(JAVA_LIB_DIR ${__java_library} PATH)
    IF(EXISTS ${JAVA_LIB_DIR}/xawt)
      LINK_DIRECTORIES(${JAVA_LIB_DIR}/xawt)
    ENDIF(EXISTS ${JAVA_LIB_DIR}/xawt)
    IF(EXISTS ${JAVA_LIB_DIR}/client)
      LINK_DIRECTORIES(${JAVA_LIB_DIR}/client)
    ENDIF(EXISTS ${JAVA_LIB_DIR}/client)
    IF(EXISTS ${JAVA_LIB_DIR}/server)
      LINK_DIRECTORIES(${JAVA_LIB_DIR}/server)
    ENDIF(EXISTS ${JAVA_LIB_DIR}/server)
  ENDFOREACH(__java_library)
  ENDIF(JAVA_AWT_LIBRARY)
  
  include_directories(
    ${JAVA_INCLUDE_PATH}
    ${JAVA_INCLUDE_PATH2}
    )
endif()

function(vtk_add_java_wrapping module_name module_srcs)

  string(REGEX REPLACE "^vtk" "" wrap_name "${module_name}")
  # FIXME: These must be here for now, should be fixed in the wrap hierarchy stuff
  set(KIT_HIERARCHY_FILE ${CMAKE_CURRENT_BINARY_DIR}/${module_name}Hierarchy.txt)

  vtk_wrap_java3(vtk${wrap_name}Java ModuleJava_SRCS "${module_srcs};${Kit_JAVA_EXTRA_WRAP_SRCS}")

  add_library(vtk${wrap_name}Java SHARED ${ModuleJava_SRCS} ${Kit_JAVA_EXTRA_SRCS})
  # Force JavaClasses to build in the right order by adding a depenency.
  add_dependencies(vtk${wrap_name}JavaJavaClasses vtk${wrap_name}Java)
  if(VTK_MODULE_${module_name}_IMPLEMENTS)
    set_property(TARGET ${module_name}Java PROPERTY COMPILE_DEFINITIONS
      "${module_name}_AUTOINIT=1(${module_name})")
  endif()

  target_link_libraries(vtk${wrap_name}Java ${module_name} vtkWrappingJavaCore)
  include_directories(${vtkWrappingJavaCore_SOURCE_DIR} 
    ${vtkWrappingJavaCore_BINARY_DIR}
    )

  # Do we need to link to AWT?
  if(${module_name} STREQUAL "vtkRenderingCore")
    target_link_libraries(vtk${wrap_name}Java ${JAVA_AWT_LIBRARY})
  endif()

  foreach(dep ${VTK_MODULE_${module_name}_DEPENDS})
    if(NOT VTK_MODULE_${dep}_EXCLUDE_FROM_WRAPPING)
      target_link_libraries(vtk${wrap_name}Java ${dep}Java)
    endif()
  endforeach()

  IF(NOT VTK_INSTALL_NO_LIBRARIES)
    INSTALL(TARGETS vtk${wrap_name}Java
     EXPORT ${VTK_INSTALL_EXPORT_NAME}
     RUNTIME DESTINATION ${VTK_INSTALL_BIN_DIR_CM24} COMPONENT RuntimeLibraries
     LIBRARY DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT RuntimeLibraries
     ARCHIVE DESTINATION ${VTK_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ENDIF(NOT VTK_INSTALL_NO_LIBRARIES)
  add_dependencies(vtk${wrap_name}Java vtk${wrap_name})
endfunction()
