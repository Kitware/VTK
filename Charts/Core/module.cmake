vtk_module(vtkChartsCore
  GROUPS
    StandAlone
  DEPENDS
    vtkRenderingContext2D
  PRIVATE_DEPENDS
    vtkCommonColor
    vtkInfovisCore # Needed for plot parallel coordinates vtkStringToCategory
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkViewsContext2D
    vtkRenderingContext${VTK_RENDERING_BACKEND}
    #vtkIOExport
    vtkIOInfovis
  )
