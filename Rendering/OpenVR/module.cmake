if (VTK_OPENVR_OBJECT_FACTORY)
  set (_impl IMPLEMENTS vtkRenderingOpenGL2)
endif()

vtk_module(vtkRenderingOpenVR
  BACKEND
    OpenGL2
  DEPENDS
    vtksys
    vtkCommonCore
    vtkRenderingOpenGL2
    vtkRenderingVolumeOpenGL2
    vtkInteractionStyle
    vtkIOImage
    vtkIOXMLParser
  ${_impl}
  PRIVATE_DEPENDS
    vtkglew
    vtkImagingSources
    vtkFiltersSources
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionWidgets
    vtkRenderingVolume
    vtkIOPLY
    vtkIOXML
)
