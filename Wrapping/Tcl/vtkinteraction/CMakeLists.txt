IF(NOT VTK_INSTALL_NO_RUNTIME)

  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings.tcl
    ${CMAKE_CURRENT_BINARY_DIR}/bindings.tcl
    COPYONLY)
    
  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings-iw.tcl
    ${CMAKE_CURRENT_BINARY_DIR}/bindings-iw.tcl
    COPYONLY)

  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings-rw.tcl
    ${CMAKE_CURRENT_BINARY_DIR}/bindings-rw.tcl
    COPYONLY)

  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/Interactor.tcl
    ${CMAKE_CURRENT_BINARY_DIR}/Interactor.tcl
    COPYONLY)
 
  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/setget.tcl
    ${CMAKE_CURRENT_BINARY_DIR}/setget.tcl
    COPYONLY)

  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkinteraction.tcl.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkinteraction.tcl
    @ONLY)

  INSTALL(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/vtkinteraction.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/Interactor.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings-iw.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings-rw.tcl
    ${CMAKE_CURRENT_SOURCE_DIR}/setget.tcl
    DESTINATION ${VTK_TCL_INSTALL_DIR_CM24}/vtkinteraction
    COMPONENT RuntimeLibraries)
ENDIF(NOT VTK_INSTALL_NO_RUNTIME)
