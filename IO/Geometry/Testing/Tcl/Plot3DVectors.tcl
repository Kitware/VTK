package require vtk
package require vtkinteraction

#
# All Plot3D vector functions
#

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderWindow renWin
    renWin SetMultiSamples 0
vtkRenderer ren1
  ren1 SetBackground .8 .8 .2
renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

set vectorLabels  "Velocity Vorticity Momentum Pressure_Gradient"
set vectorFunctions  "200 201 202 210"
vtkCamera camera
vtkLight light

# All text actors will share the same text prop

vtkTextProperty textProp
  textProp SetFontSize 10
  textProp SetFontFamilyToArial
  textProp SetColor .3 1 1

set i 0
foreach vectorFunction $vectorFunctions {
  vtkMultiBlockPLOT3DReader pl3d$vectorFunction
    pl3d$vectorFunction SetXYZFileName "$VTK_DATA_ROOT/Data/bluntfinxyz.bin"
    pl3d$vectorFunction SetQFileName "$VTK_DATA_ROOT/Data/bluntfinq.bin"
    pl3d$vectorFunction SetVectorFunctionNumber [expr int($vectorFunction)]
    pl3d$vectorFunction Update
    set output [[pl3d$vectorFunction GetOutput] GetBlock 0]
vtkStructuredGridGeometryFilter plane$vectorFunction
    plane$vectorFunction SetInputData $output
    plane$vectorFunction SetExtent 25 25 0 100 0 100
vtkHedgeHog hog$vectorFunction
  hog$vectorFunction SetInputConnection [plane$vectorFunction GetOutputPort]
  set maxnorm [[[$output GetPointData] GetVectors] GetMaxNorm]
  hog$vectorFunction SetScaleFactor [expr 1.0 / $maxnorm]
vtkPolyDataMapper mapper$vectorFunction
    mapper$vectorFunction SetInputConnection [hog$vectorFunction GetOutputPort]
vtkActor actor$vectorFunction
    actor$vectorFunction SetMapper mapper$vectorFunction

vtkRenderer ren$vectorFunction
  ren$vectorFunction SetBackground 0.5 .5 .5
  ren$vectorFunction SetActiveCamera camera
  ren$vectorFunction AddLight light
  renWin AddRenderer ren$vectorFunction

ren$vectorFunction AddActor actor$vectorFunction

vtkTextMapper textMapper$vectorFunction
  textMapper$vectorFunction SetInput [lindex $vectorLabels $i]
  textMapper$vectorFunction SetTextProperty textProp
vtkActor2D text$vectorFunction
  text$vectorFunction SetMapper textMapper$vectorFunction
  text$vectorFunction SetPosition 2 5

  if { [info command "rtExMath"] == ""} {
    ren$vectorFunction AddActor2D text$vectorFunction
  }
incr i
}
#
# now layout renderers
set column 1
set row 1
set deltaX [expr 1.0/2.0]
set deltaY [expr 1.0/2.0]

foreach vectorFunction $vectorFunctions {
    ren${vectorFunction} SetViewport [expr ($column - 1) * $deltaX + ($deltaX * .05)] [expr ($row - 1) * $deltaY + ($deltaY*.05)] [expr $column * $deltaX - ($deltaX * .05)] [expr $row * $deltaY - ($deltaY * .05)]
    incr column
    if { $column > 2 } {set column 1; incr row}
}


camera SetViewUp 1 0 0
camera SetFocalPoint 0 0 0
camera SetPosition .4 -.5 -.75
ren200 ResetCamera
camera Dolly 1.25
ren200 ResetCameraClippingRange
ren201 ResetCameraClippingRange
ren202 ResetCameraClippingRange
ren210 ResetCameraClippingRange

eval light SetPosition [camera GetPosition]
eval light SetFocalPoint [camera GetFocalPoint]

renWin SetSize 350 350
renWin Render

iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .
