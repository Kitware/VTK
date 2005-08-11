IF(NOT VTK_INSTALL_NO_RUNTIME)
  INSTALL_FILES(${VTK_TCL_INSTALL_DIR}/vtktesting .tcl
    vtktesting
    backdrop
    grab
    colors
    mccases
    )
ENDIF(NOT VTK_INSTALL_NO_RUNTIME)
