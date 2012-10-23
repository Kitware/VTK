vtk_module(vtkChartsCore
  GROUPS
    StandAlone
  DEPENDS
    vtkCommonColor
    vtkRenderingContext2D
    vtkInfovisCore # Needed for plot parallel coordinates vtkStringToCategory
  TEST_DEPENDS
    vtkCommonColor
    vtkTestingCore
    vtkTestingRendering
    vtkViewsContext2D
    vtkIOExport
    vtkIOInfovis
  )
