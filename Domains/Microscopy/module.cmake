vtk_module(vtkDomainsMicroscopy
  DESCRIPTION "Readers and writers supporting whole slide images for microscopy domain"
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionImage
    vtkRenderingContextOpenGL2
  EXCLUDE_FROM_ALL
  DEPENDS
    vtkIOImage
  PRIVATE_DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
  )
