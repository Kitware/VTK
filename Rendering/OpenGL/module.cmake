vtk_module(vtkRenderingOpenGL
  GROUPS
    Rendering
  IMPLEMENTS
    vtkRenderingCore
  DEPENDS
    # These are likely to be removed soon - split Rendering/OpenGL further.
    vtkImagingHybrid # For vtkSampleFunction
  COMPILE_DEPENDS
    vtkParseOGLExt
    vtkUtilitiesEncodeString
  TEST_DEPENDS
    vtkInteractionStyle
    vtkTestingRendering
    vtkIOExport
    vtkIOLegacy
    vtkIOXML
    vtkRenderingFreeTypeOpenGL
    vtkRenderingImage
    vtkRenderingLOD
    vtkRenderingLabel
    vtkImagingGeneral
    vtkImagingSources
    vtkFiltersProgrammable
    vtkRenderingAnnotation
  )
