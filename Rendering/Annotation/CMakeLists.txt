set(classes
  vtkAnnotatedCubeActor
  vtkArcPlotter
  vtkAxesActor
  vtkAxisActor
  vtkAxisActor2D
  vtkAxisFollower
  vtkBarChartActor
  vtkCaptionActor2D
  vtkConvexHull2D
  vtkCornerAnnotation
  vtkCubeAxesActor
  vtkCubeAxesActor2D
  vtkGraphAnnotationLayersFilter
  vtkLeaderActor2D
  vtkLegendBoxActor
  vtkLegendScaleActor
  vtkParallelCoordinatesActor
  vtkPieChartActor
  vtkPolarAxesActor
  vtkProp3DAxisFollower
  vtkScalarBarActor
  vtkSpiderPlotActor
  vtkXYPlotActor)

set(headers
  vtkScalarBarActorInternal.h)

vtk_module_add_module(VTK::RenderingAnnotation
  CLASSES ${classes}
  HEADERS ${headers})
