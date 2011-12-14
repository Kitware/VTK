ADD_SUBDIRECTORY(Cxx)

#IF (VTK_WRAP_TCL)
#  ADD_SUBDIRECTORY(Tcl)
#ENDIF (VTK_WRAP_TCL)

IF(VTK_WRAP_PYTHON)
  ADD_SUBDIRECTORY(Python)
ENDIF(VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Charts ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Charts"
    VTK_CHARTS_EXPORT
    vtkContextKeyEvent.h
    vtkContextMouseEvent.h
    vtkContextScenePrivate.h
    vtkOpenGLContextDevice2DPrivate.h
    )
ENDIF(PYTHON_EXECUTABLE)
