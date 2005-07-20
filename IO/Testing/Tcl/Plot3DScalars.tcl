package require vtk
package require vtkinteraction

#
# All Plot3D scalar functions
#

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderWindow renWin
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

set scalarLabels  "Density Pressure Temperature Enthalpy Internal_Energy Kinetic_Energy Velocity_Magnitude Stagnation_Energy Entropy Swirl"
set scalarFunctions  "100 110 120 130 140 144 153 163 170 184"
vtkCamera camera
vtkLight light
vtkMath math

# All text actors will share the same text prop

vtkTextProperty textProp
  textProp SetFontSize 10
  textProp SetFontFamilyToArial
  textProp SetColor 0 0 0

set i 0
foreach scalarFunction $scalarFunctions {
  vtkPLOT3DReader pl3d$scalarFunction
    pl3d$scalarFunction SetXYZFileName "$VTK_DATA_ROOT/Data/bluntfinxyz.bin"
    pl3d$scalarFunction SetQFileName "$VTK_DATA_ROOT/Data/bluntfinq.bin"
    pl3d$scalarFunction SetScalarFunctionNumber [expr int($scalarFunction)]
    pl3d$scalarFunction Update
vtkStructuredGridGeometryFilter plane$scalarFunction
    plane$scalarFunction SetInputConnection [pl3d$scalarFunction GetOutputPort]
    plane$scalarFunction SetExtent 25 25 0 100 0 100
vtkPolyDataMapper mapper$scalarFunction
    mapper$scalarFunction SetInputConnection [plane$scalarFunction GetOutputPort]
    eval mapper$scalarFunction SetScalarRange \
      [[[[pl3d$scalarFunction GetOutput] GetPointData] GetScalars] GetRange]
vtkActor actor$scalarFunction
    actor$scalarFunction SetMapper mapper$scalarFunction

vtkRenderer ren$scalarFunction
  ren$scalarFunction SetBackground 0 0 .5
  ren$scalarFunction SetActiveCamera camera
  ren$scalarFunction AddLight light
  renWin AddRenderer ren$scalarFunction
    ren$scalarFunction SetBackground [math Random .5 1] [math Random .5 1] [math Random .5 1] 
ren$scalarFunction AddActor actor$scalarFunction

vtkTextMapper textMapper$scalarFunction
  textMapper$scalarFunction SetInput [lindex $scalarLabels $i]
  textMapper$scalarFunction SetTextProperty textProp
vtkActor2D text$scalarFunction
  text$scalarFunction SetMapper textMapper$scalarFunction
  text$scalarFunction SetPosition 2 3

    if { [info command "rtExMath"] == ""} {
	ren$scalarFunction AddActor2D text$scalarFunction
    }
incr i
}
#
# now layout renderers
set column 1
set row 1
set deltaX [expr 1.0/5.0]
set deltaY [expr 1.0/2.0]

foreach scalarFunction $scalarFunctions {
    ren${scalarFunction} SetViewport [expr ($column - 1) * $deltaX] [expr ($row - 1) * $deltaY] [expr $column * $deltaX] [expr $row * $deltaY]
    incr column
    if { $column > 5 } {set column 1; incr row}
}

camera SetViewUp 0 1 0
camera SetFocalPoint 0 0 0
camera SetPosition 1 0 0
ren100 ResetCamera
camera Dolly 1.25
ren100 ResetCameraClippingRange
ren110 ResetCameraClippingRange
ren120 ResetCameraClippingRange
ren130 ResetCameraClippingRange
ren140 ResetCameraClippingRange
ren144 ResetCameraClippingRange
ren153 ResetCameraClippingRange
ren163 ResetCameraClippingRange
ren170 ResetCameraClippingRange
ren184 ResetCameraClippingRange

eval light SetPosition [camera GetPosition]
eval light SetFocalPoint [camera GetFocalPoint]

renWin SetSize 600 180
renWin Render
iren Initialize

# render the image
#
iren AddObserver UserEvent {wm deiconify .vtkInteract}

# prevent the tk window from showing up then start the event loop
wm withdraw .


