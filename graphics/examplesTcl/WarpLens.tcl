catch {load vtktcl}
if { [catch {set VTK_TCL $env(VTK_TCL)}] != 0} { set VTK_TCL "../../examplesTcl" }
if { [catch {set VTK_DATA $env(VTK_DATA)}] != 0} { set VTK_DATA "../../../vtkdata" }


# get the interactor ui
source $VTK_TCL/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# load in the texture map
#
vtkPNMReader pnmReader
pnmReader SetFileName "$VTK_DATA/masonry.ppm"

vtkGeometryFilter gf
gf SetInput [pnmReader GetOutput]

vtkWarpLens wl
wl SetInput [gf GetOutput]
wl SetCenter 127.5 127.5
wl SetKappa -6.0e-6

vtkTriangleFilter tf
tf SetInput [wl GetPolyDataOutput]

vtkStripper strip
strip SetInput [tf GetOutput]

vtkPolyDataMapper dsm
dsm SetInput [strip GetOutput]

vtkActor planeActor
planeActor SetMapper dsm

# Add the actors to the renderer, set the background and size
ren1 AddActor planeActor
ren1 SetBackground 0.1 0.2 0.4
renWin SetSize 500 500

# render the image
iren Initialize
iren SetUserMethod {wm deiconify .vtkInteract}
renWin Render
[ren1 GetActiveCamera] Zoom 1.4
renWin Render

# prevent the tk window from showing up then start the event loop
wm withdraw .





