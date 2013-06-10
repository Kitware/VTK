vtk_module(vtkFiltersModeling
  GROUPS
    StandAlone
  DEPENDS
    vtkFiltersGeneral
    vtkFiltersSources
  TEST_DEPENDS
    vtkCommonColor # For vtkBandedPolyDataContourFilter used in a test
    vtkIOXML
    vtkRenderingOpenGL
    vtkTestingRendering
    vtkInteractionStyle
  )
