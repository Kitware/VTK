IF(NOT VTK_INSTALL_NO_RUNTIME)

  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/vtkvolumerendering.tcl.in
    ${CMAKE_CURRENT_BINARY_DIR}/vtkvolumerendering.tcl
    @ONLY)

  INSTALL(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/vtkvolumerendering.tcl
    DESTINATION ${VTK_TCL_INSTALL_DIR_CM24}/vtkvolumerendering
    COMPONENT RuntimeLibraries)
ENDIF(NOT VTK_INSTALL_NO_RUNTIME)
