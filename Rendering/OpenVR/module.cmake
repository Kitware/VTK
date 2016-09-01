if (VTK_OPENVR_OBJECT_FACTORY)
  set (_impl IMPLEMENTS vtkRenderingOpenGL2)
endif()

vtk_module(vtkRenderingOpenVR
  BACKEND
    OpenGL2
  DEPENDS
    vtkCommonCore
    vtkRenderingOpenGL2
    vtkInteractionStyle
  ${_impl}
  PRIVATE_DEPENDS
    vtkglew
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionWidgets
    vtkIOPLY
    vtkIOXML
)
