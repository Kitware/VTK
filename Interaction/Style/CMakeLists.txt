set(Module_SRCS
  vtkInteractorStyleDrawPolygon.cxx
  vtkInteractorStyleFlight.cxx
  vtkInteractorStyleImage.cxx
  vtkInteractorStyleJoystickActor.cxx
  vtkInteractorStyleJoystickCamera.cxx
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

set_source_files_properties(
  ${vtk-module}ObjectFactory
  WRAP_EXCLUDE
  )

# Now to generate our object factory.
set(vtk_module_overrides "vtkInteractorStyleSwitchBase")
set(vtk_module_vtkInteractorStyleSwitchBase_override "vtkInteractorStyleSwitch")

# Now we iterate and create that class file...
foreach(_class ${vtk_module_overrides})
  set(_override ${vtk_module_${_class}_override})
  set(_vtk_override_includes "${_vtk_override_includes}
#include \"${_override}.h\"")
  set(_vtk_override_creates "${_vtk_override_creates}
VTK_CREATE_CREATE_FUNCTION(${_override})")
  set(_vtk_override_do "${_vtk_override_do}
  this->RegisterOverride(\"${_class}\",
                         \"${_override}\",
                         \"Override for ${vtk-module} module\", 1,
                         vtkObjectFactoryCreate${_override});")
endforeach()

# Now lets create the object factory classes
string(TOUPPER ${vtk-module} VTK-MODULE)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.h)
configure_file(${VTK_CMAKE_DIR}/vtkObjectFactory.cxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/${vtk-module}ObjectFactory.cxx)

vtk_module_library(vtkInteractionStyle ${Module_SRCS})
