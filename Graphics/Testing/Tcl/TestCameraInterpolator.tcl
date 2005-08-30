package require vtk
package require vtkinteraction
package require vtktesting

vtkLookupTable lut
  lut SetHueRange 0.6 0
  lut SetSaturationRange 1.0 0
  lut SetValueRange 0.5 1.0

# Read the data: a height field results
vtkDEMReader demReader
  demReader SetFileName "$VTK_DATA_ROOT/Data/SainteHelens.dem"
  demReader Update

set lo [lindex [[demReader GetOutput] GetScalarRange] 0]
set hi [lindex [[demReader GetOutput] GetScalarRange] 1]

vtkImageDataGeometryFilter surface
  surface SetInputConnection [demReader GetOutputPort]

vtkWarpScalar warp
  warp SetInputConnection [surface GetOutputPort]
  warp SetScaleFactor 1
  warp UseNormalOn
  warp SetNormal 0 0 1

vtkPolyDataNormals normals
  normals SetInput [warp GetPolyDataOutput]
  normals SetFeatureAngle 60
  normals SplittingOff

vtkPolyDataMapper demMapper
  demMapper SetInputConnection [normals GetOutputPort]
  eval demMapper SetScalarRange $lo $hi
  demMapper SetLookupTable lut

vtkLODActor demActor
  demActor SetMapper demMapper
  
# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor demActor

ren1 SetBackground 0 0 0
renWin SetSize 300 300
ren1 SetBackground 0.1 0.2 0.4

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .

vtkCamera view1
view1 SetClippingRange 30972.2 35983.7
view1 SetFocalPoint 562835 5.11498e+006 2294.5
view1 SetPosition 562835 5.11498e+006 35449.9
view1 SetViewAngle 30
view1 SetViewUp 0 1 0

vtkCamera view2
view2 SetClippingRange 9013.43 13470.4
view2 SetFocalPoint 562835 5.11498e+006 2294.5
view2 SetPosition 562835 5.11498e+006 13269.4
view2 SetViewAngle 30
view2 SetViewUp 0 1 0

vtkCamera view3
view3 SetClippingRange 4081.2 13866.4
view3 SetFocalPoint 562853 5.11586e+006 2450.05
view3 SetPosition 562853 5.1144e+006 10726.6
view3 SetViewAngle 30
view3 SetViewUp 0 0.984808 0.173648

vtkCamera view4
view4 SetClippingRange 14.0481 14048.1
view4 SetFocalPoint 562880 5.11652e+006 2733.15
view4 SetPosition 562974 5.11462e+006 6419.98
view4 SetViewAngle 30
view4 SetViewUp 0.0047047 0.888364 0.459116

vtkCamera view5
view5 SetClippingRange 14.411 14411
view5 SetFocalPoint 562910 5.11674e+006 3027.15
view5 SetPosition 562414 5.11568e+006 3419.87
view5 SetViewAngle 30
view5 SetViewUp -0.0301976 0.359864 0.932516

vtkCameraInterpolator interpolator
interpolator SetInterpolationTypeToSpline
interpolator AddCamera 0 view1
interpolator AddCamera 5 view2
interpolator AddCamera 7.5 view3
interpolator AddCamera 9.0 view4
interpolator AddCamera 11.0 view5

vtkCamera camera
ren1 SetActiveCamera camera
proc animate {} {
    set numSteps 500
    set min [interpolator GetMinimumT]
    set max [interpolator GetMaximumT]
    for {set i 0} {$i <= $numSteps} {incr i} {
        set t [expr double($i) * ($max - $min) / double($numSteps)]
        interpolator InterpolateCamera $t camera
        renWin Render
    }
}
interpolator InterpolateCamera 8.2 camera
#animate
