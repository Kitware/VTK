vtk_module(vtkDomainsMicroscopy
  DESCRIPTION "Readers and writers supporting whole slide images for microscopy domain"
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionImage
    vtkRenderingContext${VTK_RENDERING_BACKEND}
  EXCLUDE_FROM_ALL
  DEPENDS
    vtkIOImage
  PRIVATE_DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
  )