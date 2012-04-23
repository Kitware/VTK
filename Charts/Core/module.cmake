vtk_module(vtkChartsCore
  GROUPS
    StandAlone
  DEPENDS
    vtkRenderingContext2D
    vtkInfovisCore # Needed for plot parallel coordinates vtkStringToCategory
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkViewsContext2D
    vtkIOInfovis
  )
