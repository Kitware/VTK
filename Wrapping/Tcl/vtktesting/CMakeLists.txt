IF(NOT VTK_INSTALL_NO_RUNTIME)
  INSTALL_FILES(${VTK_INSTALL_LIB_DIR}/tcl/vtktesting .tcl
    vtktesting
    backdrop
    grab
    colors
    mccases
    )
ENDIF(NOT VTK_INSTALL_NO_RUNTIME)
