include(vtkObjectFactory)

set(Module_SRCS
  vtkInteractorStyleDrawPolygon.cxx
  vtkInteractorStyleFlight.cxx
  vtkInteractorStyleImage.cxx
  vtkInteractorStyleJoystickActor.cxx
  vtkInteractorStyleJoystickCamera.cxx
  vtkInteractorStyleMultiTouchCamera.cxx
  vtkInteractorStyleRubberBand2D.cxx
  vtkInteractorStyleRubberBand3D.cxx
  vtkInteractorStyleRubberBandPick.cxx
  vtkInteractorStyleRubberBandZoom.cxx
  vtkInteractorStyleTerrain.cxx
  vtkInteractorStyleTrackballActor.cxx
  vtkInteractorStyleTrackballCamera.cxx
  vtkInteractorStyleTrackball.cxx
  vtkInteractorStyleUnicam.cxx
  vtkInteractorStyleUser.cxx

  vtkInteractorStyleSwitch.cxx
  vtkParallelCoordinatesInteractorStyle.cxx

  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx
  )

set_source_files_properties(${vtk-module}ObjectFactory WRAP_EXCLUDE)

# Now to generate our object factory.
set(vtk_module_overrides "vtkInteractorStyleSwitchBase")
if (ANDROID OR APPLIE_IOS)
  set(vtk_module_vtkInteractorStyleSwitchBase_override "vtkInteractorStyleMultiTouchCamera")
else()
  set(vtk_module_vtkInteractorStyleSwitchBase_override "vtkInteractorStyleSwitch")
endif()
vtk_object_factory_configure("${vtk_module_overrides}")

vtk_module_library(vtkInteractionStyle ${Module_SRCS})
