ADD_SUBDIRECTORY(Cxx)

IF (VTK_WRAP_TCL)
  ADD_SUBDIRECTORY(Tcl)
ENDIF (VTK_WRAP_TCL)

IF (VTK_WRAP_PYTHON)
  ADD_SUBDIRECTORY(Python)
ENDIF (VTK_WRAP_PYTHON)

IF(PYTHON_EXECUTABLE)
  ADD_TEST(HeaderTesting-Rendering ${PYTHON_EXECUTABLE}
    ${VTK_SOURCE_DIR}/Common/Testing/HeaderTesting.py
    "${VTK_SOURCE_DIR}/Rendering"
    VTK_RENDERING_EXPORT
    vtkCocoaMacOSXSDKCompatibility.h
    vtkCocoaGLView.h
    vtkFreeTypeUtilities.h
    vtkgl.h
    vtkgluPickMatrix.h
    vtkGPUInfoListArray.h
    vtkLabelHierarchyPrivate.h
    vtkNoise200x200.h
    vtkOpenGL.h
    vtkOpenGLExtensionManagerConfigure.h
    vtkOpenGLState.h
    vtkPythagoreanQuadruples.h
    vtkRenderState.h
    vtkShadowMapPassInternal.h
    vtkTDxMotionEventInfo.h
    vtkTkImageViewerWidget.h
    vtkTkInternals.h
    vtkTkRenderWidget.h
    )
ENDIF(PYTHON_EXECUTABLE)
