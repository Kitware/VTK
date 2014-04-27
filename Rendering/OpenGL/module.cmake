# A hack to let us inject OpenGL2 as a dependency, and change overrides.
unset(_injectOpenGL2)
if(VTK_REPLACE_OPENGL_OVERRIDES)
  set(_injectOpenGL2 vtkRenderingOpenGL2)
endif()

vtk_module(vtkRenderingOpenGL
  GROUPS
    Rendering
  IMPLEMENTS
    vtkRenderingCore
  PRIVATE_DEPENDS
    # These are likely to be removed soon - split Rendering/OpenGL further.
    vtkImagingHybrid # For vtkSampleFunction
    vtksys
    ${_injectOpenGL2}
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
