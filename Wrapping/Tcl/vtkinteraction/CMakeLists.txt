IF(NOT VTK_INSTALL_NO_RUNTIME)
  INSTALL_FILES(${VTK_TCL_INSTALL_DIR}/vtkinteraction .tcl
    vtkinteraction
    Interactor
    bindings
    bindings-iw
    bindings-rw
    setget
    )
ENDIF(NOT VTK_INSTALL_NO_RUNTIME)
