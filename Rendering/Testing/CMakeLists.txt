SUBDIRS(Cxx)

IF (VTK_WRAP_TCL)
  SUBDIRS(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  SUBDIRS(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Rendering ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Rendering"
    VTK_RENDERING_EXPORT
    vtkTkRenderWidget.h
    vtkTkInternals.h
    vtkTkImageViewerWidget.h
    vtkCocoaGLView.h
    vtkFreeTypeUtilities.h
    vtkgluPickMatrix.h
    vtkOpenGL.h
    vtkOpenGLStateCache.h
    vtkgl.h
    vtkOpenGLExtensionManagerConfigure.h
    )
ENDIF(PYTHON_EXECUTABLE)
