# The resources dir is only worth exporting if we are building with
# Tcl/Tk static
IF(VTK_USE_TK AND VTK_TCL_TK_STATIC)
  IF(VTK_TK_RESOURCES_DIR)
    IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
      FILE(GLOB tkResourceFiles "*.bmp" "*.cur" "*.ico" "*.manifest" "*.rc")
      INSTALL(FILES
        ${tkResourceFiles}
        DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/TclTk/resources/tk8.6/win/rc
        COMPONENT Development
        )
    ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  ENDIF(VTK_TK_RESOURCES_DIR)
ENDIF(VTK_USE_TK AND VTK_TCL_TK_STATIC)
