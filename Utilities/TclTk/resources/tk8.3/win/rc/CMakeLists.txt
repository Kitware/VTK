# The resources dir is only worth exporting if we are building with
# Tcl/Tk static
IF(VTK_USE_TK AND VTK_TCL_TK_STATIC)
  IF(VTK_TK_RESOURCES_DIR)
    IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
      INSTALL_FILES(${VTK_INSTALL_INCLUDE_DIR}/TclTk/resources/tk8.3/win/rc 
        "\\.(bmp|cur|ico|manifest|rc)$")
    ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  ENDIF(VTK_TK_RESOURCES_DIR)
ENDIF(VTK_USE_TK AND VTK_TCL_TK_STATIC)
