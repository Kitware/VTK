catch {load vtktcl}
# get the interactor ui
source vtkInt.tcl
source "colors.tcl"

# cut data
vtkPLOT3DReader pl3d
    pl3d SetXYZFileName "../../../data/combxyz.bin"
    pl3d SetQFileName "../../../data/combq.bin"
    pl3d SetScalarFunctionNumber 100
    pl3d SetVectorFunctionNumber 202
    pl3d Update
set range [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange]
set min [lindex $range 0]
set max [lindex $range 1]
set value [expr ($min + $max) / 2.0]

vtkContourFilter cf
    cf SetInput [pl3d GetOutput]
    cf SetValue 0 $value
vtkPolyMapper cutMapper
    cutMapper SetInput [cf GetOutput]
    eval cutMapper SetScalarRange \
      [[[[pl3d GetOutput] GetPointData] GetScalars] GetRange]
vtkActor cutActor
    cutActor SetMapper cutMapper

#outline
vtkStructuredGridOutlineFilter outline
    outline SetInput [pl3d GetOutput]
vtkPolyMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper
    eval [outlineActor GetProperty] SetColor 0 0 0

## Graphics stuff
#
vtkRenderMaster rm

# Now create the RenderWindow, Renderer and both Actors
#
set renWin [rm MakeRenderWindow]
set ren1   [$renWin MakeRenderer]
set iren [$renWin MakeRenderWindowInteractor]
# Add the actors to the renderer, set the background and size
#
$ren1 AddActor outlineActor
$ren1 AddActor cutActor
$ren1 SetBackground 1 1 1
$renWin SetSize 500 500

set cam1 [$ren1 GetActiveCamera]
$cam1 SetClippingRange 3.95297 50
$cam1 SetFocalPoint 9.71821 0.458166 29.3999
$cam1 SetPosition 2.7439 -37.3196 38.7167
$cam1 ComputeViewPlaneNormal
$cam1 SetViewUp -0.16123 0.264271 0.950876
$iren Initialize

# render the image
#
$iren SetUserMethod {wm deiconify .vtkInteract}

# loop over surfaces
for {set nloops 0} {$nloops < 3} {incr nloops} {
    for {set i 0} {$i < 17} {incr i} {
      cf SetValue 0 [expr $min + ($i/16.0)*($max - $min)]
      $renWin Render
    }
}

# prevent the tk window from showing up then start the event loop
wm withdraw .

