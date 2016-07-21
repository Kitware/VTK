vtk_module(vtkDomainsMicroscopy
  DESCRIPTION "Readers and writers supporting whole slide images for microscopy domain"
  DEPENDS
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkIOImage
  PRIVATE_DEPENDS
    vtkIOXML
    vtkFiltersSources
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionImage
    vtkRenderingContext${VTK_RENDERING_BACKEND}
  EXCLUDE_FROM_ALL
  )
