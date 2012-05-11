set(Module_SRCS
  vtkAnnotatedCubeActor.cxx
  vtkArcPlotter.cxx
  vtkAxesActor.cxx
  vtkAxisActor2D.cxx
  vtkAxisActor.cxx
  vtkAxisFollower.cxx
  vtkBarChartActor.cxx
  vtkCaptionActor2D.cxx
  vtkCornerAnnotation.cxx
  vtkCubeAxesActor2D.cxx
  vtkCubeAxesActor.cxx
  vtkLeaderActor2D.cxx
  vtkLegendBoxActor.cxx
  vtkLegendScaleActor.cxx
  vtkPieChartActor.cxx
  vtkPolarAxesActor.cxx
  vtkScalarBarActor.cxx
  vtkSpiderPlotActor.cxx
  vtkXYPlotActor.cxx
  )

vtk_module_library(vtkRenderingAnnotation ${Module_SRCS})
