catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }

# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

vtkUGFacetReader ugReader
    ugReader SetFileName $VTK_DATA/bolt.fac
    ugReader MergingOff

vtkPolyDataMapper   ugMapper
    ugMapper SetInput [ugReader GetOutput]
vtkActor ugActor
    ugActor SetMapper ugMapper

# Add the actors to the renderer, set the background and size
#
ren1 AddActor ugActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
#
iren SetUserMethod {wm deiconify .vtkInteract}
set cam1 [ren1 GetActiveCamera]
$cam1 Elevation 210
$cam1 Azimuth 30
ren1 ResetCameraClippingRange

renWin Render
#renWin SetFileName "ugFacet.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
wm withdraw .
