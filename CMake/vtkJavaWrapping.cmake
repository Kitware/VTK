if(VTK_WRAP_JAVA)
  set(VTK_WRAP_JAVA3_INIT_DIR "${VTK_SOURCE_DIR}/Wrapping/Java")
  find_package(Java)
  find_package(JNI)
  include(vtkWrapJava)

  IF(JAVA_AWT_LIBRARY)
  FOREACH(__java_library ${JAVA_AWT_LIBRARY})
    GET_FILENAME_COMPONENT(JAVA_LIB_DIR ${__java_library} PATH)
    IF(EXISTS ${JAVA_LIB_DIR}/xawt)
      LINK_DIRECTORIES(${JAVA_LIB_DIR}/xawt)
    ENDIF()
    IF(EXISTS ${JAVA_LIB_DIR}/client)
      LINK_DIRECTORIES(${JAVA_LIB_DIR}/client)
    ENDIF()
    IF(EXISTS ${JAVA_LIB_DIR}/server)
      LINK_DIRECTORIES(${JAVA_LIB_DIR}/server)
    ENDIF()
  ENDFOREACH()
  ENDIF()

endif()

function(vtk_add_java_wrapping module_name module_srcs module_hdrs)

  set(_java_include_dirs
    ${vtkWrappingJava_BINARY_DIR}
    ${vtkWrappingJava_SOURCE_DIR}
    ${JAVA_INCLUDE_PATH}
    ${JAVA_INCLUDE_PATH2})

  if(NOT CMAKE_HAS_TARGET_INCLUDES)
    include_directories(${_java_include_dirs})
  endif()

  if(NOT ${module_name}_EXCLUDE_FROM_WRAP_HIERARCHY)
    set(KIT_HIERARCHY_FILE ${${module_name}_WRAP_HIERARCHY_FILE})
  endif()

  vtk_wrap_java3(${module_name}Java ModuleJava_SRCS
    "${module_srcs};${Kit_JAVA_EXTRA_WRAP_SRCS}")

  add_library(${module_name}Java SHARED ${ModuleJava_SRCS} ${Kit_JAVA_EXTRA_SRCS})
  if(MINGW)
    set_target_properties(${module_name}Java PROPERTIES PREFIX "")
  endif()
  vtk_target_export(${module_name}Java)
  if(CMAKE_HAS_TARGET_INCLUDES)
    set_property(TARGET ${module_name}Java APPEND
      PROPERTY INCLUDE_DIRECTORIES ${_java_include_dirs})
  endif()
  # Force JavaClasses to build in the right order by adding a depenency.
  add_dependencies(${module_name}JavaJavaClasses ${module_name}Java)
  if(${module_name}_IMPLEMENTS)
    set_property(TARGET ${module_name}Java PROPERTY COMPILE_DEFINITIONS
      "${module_name}_AUTOINIT=1(${module_name})")
  endif()

  target_link_libraries(${module_name}Java LINK_PUBLIC ${module_name} vtkWrappingJava)

  # Do we need to link to AWT?
  if(${module_name} STREQUAL "vtkRenderingCore")
    target_link_libraries(${module_name}Java LINK_PUBLIC ${JAVA_AWT_LIBRARY})
    if(APPLE)
      target_link_libraries(${module_name}Java LINK_PUBLIC "-framework Cocoa")
    endif()
  endif()

  foreach(dep ${${module_name}_WRAP_DEPENDS})
    if(NOT ${dep}_EXCLUDE_FROM_WRAPPING)
      target_link_libraries(${module_name}Java LINK_PUBLIC ${dep}Java)
    endif()
  endforeach()

  if(NOT VTK_INSTALL_NO_LIBRARIES)
    if(APPLE AND VTK_JAVA_INSTALL)
      set_target_properties(${module_name}Java PROPERTIES SUFFIX ".jnilib")
    endif()
    install(TARGETS ${module_name}Java
      EXPORT ${VTK_INSTALL_EXPORT_NAME}
      RUNTIME DESTINATION ${VTK_INSTALL_RUNTIME_DIR} COMPONENT RuntimeLibraries
      LIBRARY DESTINATION ${VTK_INSTALL_LIBRARY_DIR} COMPONENT RuntimeLibraries
      ARCHIVE DESTINATION ${VTK_INSTALL_ARCHIVE_DIR} COMPONENT Development)
  endif()
endfunction()
