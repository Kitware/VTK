set(classes
  vtkCameraManipulator
  vtkInteractorStyleDrawPolygon
  vtkInteractorStyleFlight
  vtkInteractorStyleImage
  vtkInteractorStyleJoystickActor
  vtkInteractorStyleJoystickCamera
  vtkInteractorStyleManipulator
  vtkInteractorStyleMultiTouchCamera
  vtkInteractorStyleRubberBand2D
  vtkInteractorStyleRubberBand3D
  vtkInteractorStyleRubberBandPick
  vtkInteractorStyleRubberBandZoom
  vtkInteractorStyleSwitch
  vtkInteractorStyleTerrain
  vtkInteractorStyleTrackballActor
  vtkInteractorStyleTrackballCamera
  vtkInteractorStyleUnicam
  vtkInteractorStyleUser
  vtkJoystickFly
  vtkJoystickFlyIn
  vtkJoystickFlyOut
  vtkParallelCoordinatesInteractorStyle
  vtkTableTopRotate
  vtkTrackballEnvironmentRotate
  vtkTrackballMoveActor
  vtkTrackballMultiRotate
  vtkTrackballPan
  vtkTrackballRoll
  vtkTrackballRotate
  vtkTrackballZoom
  vtkTrackballZoomToMouse)

if (ANDROID OR APPLE_IOS)
  set(style_override "vtkInteractorStyleMultiTouchCamera")
else ()
  set(style_override "vtkInteractorStyleSwitch")
endif ()
vtk_object_factory_declare(
  BASE vtkInteractorStyleSwitchBase
  OVERRIDE "${style_override}")

vtk_object_factory_configure(
  SOURCE_FILE vtk_object_factory_source
  HEADER_FILE vtk_object_factory_header
  EXPORT_MACRO "VTKINTERACTIONSTYLE_EXPORT")

# Hand written (de)serialization routines
set(serialization_helper_sources)
if (VTK_WRAP_SERIALIZATION)
  list(APPEND serialization_helper_sources
    vtkInteractorStyleSwitchSerDesHelper.cxx)
endif ()

set(sources
  ${vtk_object_factory_source}
  ${serialization_helper_sources})

vtk_module_add_module(VTK::InteractionStyle
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${vtk_object_factory_header})
vtk_add_test_mangling(VTK::InteractionStyle)
