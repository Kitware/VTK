## cut a volume with cell data
#
# get the interactor ui
catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


source $VTK_TCL/vtkInt.tcl

vtkStructuredPoints volume
    volume SetDimensions 5 10 15
set numScalars [expr 4*9*14]
vtkMath math
vtkScalars cellScalars
    cellScalars SetNumberOfScalars $numScalars
for {set i 0} {$i < $numScalars} {incr i} {
    cellScalars SetScalar $i [math Random 0 1]
}
[volume GetCellData] SetScalars cellScalars

# create a sphere source and actor
#
vtkPlane plane
    eval plane SetOrigin [volume GetCenter]
    plane SetNormal 0 1 1
vtkCutter planeCut
    planeCut SetInput volume
    planeCut SetCutFunction plane
vtkPolyDataMapper   mapper
    mapper SetInput [planeCut GetOutput]
    mapper GlobalImmediateModeRenderingOn
vtkLODActor cutActor
    cutActor SetMapper mapper

vtkOutlineFilter outline
    outline SetInput volume
vtkPolyDataMapper outlineMapper
    outlineMapper SetInput [outline GetOutput]
vtkActor outlineActor
    outlineActor SetMapper outlineMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# Add the actors to the renderer, set the background and size
#
ren1 AddActor cutActor
ren1 AddActor outlineActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 300 300

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}

set cam1 [ren1 GetActiveCamera]
$cam1 SetClippingRange 3.83384 191.692
$cam1 SetFocalPoint 1.67401 3.99838 7.71124
$cam1 SetPosition 26.1644 21.623 31.3635
$cam1 SetViewUp -0.321615 0.887994 -0.328681
$cam1 Dolly 1.4

iren Initialize
renWin SetFileName "cutCells.tcl.ppm"
#renWin SaveImageAsPPM

proc TkCheckAbort {} {
  set foo [renWin GetEventPending]
  if {$foo != 0} {renWin SetAbortRender 1}
}
renWin SetAbortCheckMethod {TkCheckAbort}

# prevent the tk window from showing up then start the event loop
wm withdraw .



