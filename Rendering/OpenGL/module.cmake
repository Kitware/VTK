vtk_module(vtkRenderingOpenGL
  GROUPS
    Rendering
  IMPLEMENTS
    vtkRenderingCore
  PRIVATE_DEPENDS
    # These are likely to be removed soon - split Rendering/OpenGL further.
    vtkImagingHybrid # For vtkSampleFunction
    vtksys
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
    vtkFiltersSources
    vtkRenderingAnnotation
  )
