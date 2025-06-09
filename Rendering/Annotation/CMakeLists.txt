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
  vtkPolarAxesActor2D
  vtkProp3DAxisFollower
  vtkRadialGridActor2D
  vtkScalarBarActor
  vtkSpiderPlotActor
  vtkXYPlotActor)

set(private_classes
"Private/vtkArcGridActorInternal"
"Private/vtkAxisGridActorInternal"
"Private/vtkTextActorInterfaceInternal")

set(nowrap_headers
  vtkScalarBarActorInternal.h)

# Hand written (de)serialization routines
set(serialization_helper_sources)
if (VTK_WRAP_SERIALIZATION)
  list(APPEND serialization_helper_sources
    vtkXYPlotActorSerDesHelper.cxx)
endif ()

vtk_module_add_module(VTK::RenderingAnnotation
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  SOURCES ${serialization_helper_sources}
  NOWRAP_HEADERS ${nowrap_headers})
vtk_add_test_mangling(VTK::RenderingAnnotation)
